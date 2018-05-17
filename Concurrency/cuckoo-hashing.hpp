#pragma once

#include <tpcc/concurrency/backoff.hpp>

#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/stdlike/mutex.hpp>

#include <tpcc/logging/logging.hpp>

#include <tpcc/support/compiler.hpp>
#include <tpcc/support/random.hpp>

#include <algorithm>
#include <functional>
#include <set>
#include <utility>
#include <vector>

#include <iostream>

namespace tpcc {
namespace solutions {

////////////////////////////////////////////////////////////////////////////////

class TTASSpinLock {
 public:
  TTASSpinLock() : locked_(false) {
  }
  void lock() {
    Backoff backoff;
    while (true) {
      while (locked_.load()) {
        backoff();
      }
      if (!locked_.exchange(true)) {
        return;
      }
    }
  }

  void unlock() {
    locked_.store(false);
  }

 private:
  tpcc::atomic<bool> locked_{false};
};

////////////////////////////////////////////////////////////////////////////////

using Mutex = TTASSpinLock;
using MutexLocker = std::unique_lock<Mutex>;

class LockSet {
 public:
  LockSet& Lock(Mutex& mutex) {
    locks_.emplace_back(mutex);
    return *this;
  }

  void Unlock() {
    locks_.clear();
  }

 private:
  std::vector<MutexLocker> locks_;
};

class LockManager {
 public:
  LockManager(size_t concurrency_level) : locks_(concurrency_level) {
  }

  size_t GetLockCount() const {
    return locks_.size();
  }

  LockSet Lock(size_t i) {
    LockSet onelock;
    onelock.Lock(locks_[i]);
    return onelock;
  }

  LockSet Lock(size_t i, size_t j) {
    if (j < i) {
      std::swap(i, j);
    }
    LockSet pair;
    pair.Lock(locks_[i]);
    if (i != j) {
      pair.Lock(locks_[j]);
    }
    return pair;
  }

  LockSet LockAll() {
    LockSet set;
    for (int i = 0; i < locks_.size(); i++) {
      set.Lock(locks_[i]);
    }
    return set;
  }

 private:
  std::vector<Mutex> locks_;
};

////////////////////////////////////////////////////////////////////////////////

struct ElementHash {
  size_t hash_value_;
  uint16_t alt_value_;

  bool operator==(const ElementHash& that) const {
    return hash_value_ == that.hash_value_ && alt_value_ == that.alt_value_;
  }

  bool operator!=(const ElementHash& that) const {
    return !(*this == that);
  }
};

template <typename T, typename HashFunction>
class CuckooHasher {
 public:
  ElementHash operator()(const T& element) const {
    size_t hash_value = hasher_(element);
    return {.hash_value_ = hash_value,
            .alt_value_ = ComputeAltValue(hash_value)};
  }

  ElementHash AlternateHash(const ElementHash& hash) {
    return {.hash_value_ = hash.hash_value_ ^ hash.alt_value_,
            .alt_value_ = hash.alt_value_};
  }

 private:
  uint16_t ComputeAltValue(size_t hash_value) const {
    hash_value *= 1349110179037u;
    return ((hash_value >> 48) ^ (hash_value >> 32) ^ (hash_value >> 16) ^
            hash_value) &
           0xFFFF;
  }

 private:
  std::hash<T> hasher_;
};

////////////////////////////////////////////////////////////////////////////////

using CuckooPath = std::vector<size_t>;

////////////////////////////////////////////////////////////////////////////////

// use this exceptions to interrupt and retry current operation

struct TableOvercrowded : std::logic_error {
  TableOvercrowded() : std::logic_error("Table overcrowded") {
  }
};

struct TableExpanded : std::logic_error {
  TableExpanded() : std::logic_error("Table expanded") {
  }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, class HashFunction = std::hash<T>>
class CuckooHashSet {
 private:
  const size_t kMaxPathLength = 8;
  const size_t kFindPathIterations = 3;
  const size_t kEvictIterations = 16;

 private:
  struct CuckooBucket {
    T element_;
    ElementHash hash_;
    bool occupied_{false};

    CuckooBucket() {
    }

    CuckooBucket(T element, const ElementHash& hash)
        : element_(std::move(element)), hash_(hash), occupied_(true) {
    }

    void Set(const T& element, ElementHash hash) {
      element_ = element;
      hash_ = hash;
      occupied_ = true;
    }

    void Clear() {
      occupied_ = false;
    }

    bool IsOccupied() const {
      return occupied_;
    }
  };

  using CuckooBuckets = std::vector<CuckooBucket>;
  using TwoBuckets = std::pair<size_t, size_t>;

 public:
  explicit CuckooHashSet(const size_t concurrency_level = 32,
                         const size_t bucket_width = 4)
      : lock_manager_(concurrency_level),
        bucket_count_(GetInitialBucketCount(concurrency_level)),
        items_(0),
        buckets_(CreateEmptyTable(bucket_count_)) {
  }

  bool Insert(const T& element) {
    ElementHash hash = hasher_(element);
    while (true) {
      RememberBucketCount();
      const TwoBuckets b_pair = GetBuckets(hash);
      try {
        LockSet lockset;
        LockTwoBuckets(b_pair, lockset);
        InterruptIfTableExpanded();

        if ((buckets_[b_pair.first].IsOccupied() &&
             buckets_[b_pair.first].hash_ == hash &&
             buckets_[b_pair.first].element_ == element) ||
            (buckets_[b_pair.second].IsOccupied() &&
             buckets_[b_pair.second].hash_ == hash &&
             buckets_[b_pair.second].element_ == element)) {
          return false;
        }

        if (!buckets_[b_pair.first].IsOccupied()) {
          buckets_[b_pair.first].Set(element, hash);
          items_.fetch_add(1);
          return true;
        }
        if (!buckets_[b_pair.second].IsOccupied()) {
          buckets_[b_pair.second].Set(element, hash);
          items_.fetch_add(1);
          return true;
        }
        lockset.Unlock();
        EvictElement(b_pair);
      } catch (const TableExpanded& _) {
        // LOG_SIMPLE("Insert interrupted due to concurrent table expansion");
      } catch (const TableOvercrowded& _) {
        // LOG_SIMPLE("Cannot insert element, table overcrowded");
        ExpandTable();
      }
    }
    UNREACHABLE();
  }

  bool Remove(const T& element) {
    auto hash = hasher_(element);
    RememberBucketCount();
    while (true) {
      const TwoBuckets b_pair = GetBuckets(hash);
      LockSet lockset;
      LockTwoBuckets(b_pair, lockset);
      if (bucket_count_.load() != expected_bucket_count_) {
        RememberBucketCount();
        continue;
      }
      if (buckets_[b_pair.first].IsOccupied() &&
          buckets_[b_pair.first].hash_ == hash &&
          buckets_[b_pair.first].element_ == element) {
        buckets_[b_pair.first].Clear();
        items_.fetch_sub(1);
        return true;
      }
      if (buckets_[b_pair.second].IsOccupied() &&
          buckets_[b_pair.second].hash_ == hash &&
          buckets_[b_pair.second].element_ == element) {
        buckets_[b_pair.second].Clear();
        items_.fetch_sub(1);
        return true;
      }
      return false;
    }
    UNREACHABLE();
  }

  bool Contains(const T& element) const {
    ElementHash hash = hasher_(element);
    while (true) {
      RememberBucketCount();
      const TwoBuckets b_pair = GetBuckets(hash);
      LockSet lockset;
      LockTwoBuckets(b_pair, lockset);
      if (bucket_count_.load() != expected_bucket_count_) {
        continue;
      }
      if ((buckets_[b_pair.first].IsOccupied() &&
           buckets_[b_pair.first].hash_ == hash &&
           buckets_[b_pair.first].element_ == element) ||
          (buckets_[b_pair.second].IsOccupied() &&
           buckets_[b_pair.second].hash_ == hash &&
           buckets_[b_pair.second].element_ == element)) {
        return true;
      }
      return false;
    }
  }

  size_t GetSize() const {
    return items_.load();
  }

  double GetLoadFactor() const {
    return (double)items_.load() / (double)bucket_count_.load();
  }

 private:
  static CuckooBuckets CreateEmptyTable(const size_t bucket_count) {
    return CuckooBuckets(bucket_count);
  }

  static size_t GetInitialBucketCount(const size_t concurrency_level) {
    return concurrency_level;
  }

  size_t HashToBucket(const size_t hash_value) const {
    return hash_value % expected_bucket_count_;
  }

  size_t GetPrimaryBucket(const ElementHash& hash) const {
    return HashToBucket(hash.hash_value_);
  }

  size_t GetAlternativeBucket(const ElementHash& hash) const {
    return HashToBucket(hash.hash_value_ ^ hash.alt_value_);
  }

  size_t GetPrimaryHash(const ElementHash& hash) const {
    return hash.hash_value_;
  }

  size_t GetAlternativeHash(const ElementHash& hash) const {
    return hash.hash_value_ ^ hash.alt_value_;
  }

  TwoBuckets GetBuckets(const ElementHash& hash) const {
    return {GetPrimaryBucket(hash), GetAlternativeBucket(hash)};
  }

  void RememberBucketCount() const {
    expected_bucket_count_ = bucket_count_.load();
  }

  void InterruptIfTableExpanded() const {
    if (bucket_count_.load() != expected_bucket_count_) {
      throw TableExpanded{};
    }
  }

  size_t GetLockIndex(const size_t bucket_index) const {
    return bucket_index % lock_manager_.GetLockCount();
  }

  LockSet LockTwoBuckets(const TwoBuckets& buckets) const {
    return lock_manager_.Lock(GetLockIndex(buckets.first),
                              GetLockIndex(buckets.second));
  }

  void LockTwoBuckets(const TwoBuckets& buckets, LockSet& locks) const {
    locks = LockTwoBuckets(buckets);
  }

  bool TryMoveHoleBackward(const CuckooPath& path) {
    for (int i = path.size() - 1; i >= 1; i--) {
      const TwoBuckets current_pair(path[i], path[i - 1]);
      LockSet lockset;
      LockTwoBuckets(current_pair, lockset);
      InterruptIfTableExpanded();
      if (!buckets_[path[i]].IsOccupied()) {
        if (buckets_[path[i - 1]].IsOccupied()) {
          buckets_[path[i]] = buckets_[path[i - 1]];
          buckets_[path[i - 1]].Clear();
        }
      } else {
        return false;
      }
    }
    LockSet lockset = lock_manager_.Lock(GetLockIndex(path[0]));
    if (!buckets_[path[0]].IsOccupied()) {
      buckets_[path[0]].Clear();
    } else {
      return false;
    }
    return true;
  }

  bool TryFindCuckooPathWithRandomWalk(size_t start_bucket, CuckooPath& path) {
    size_t current_bucket = start_bucket;
    while (path.size() < std::max(bucket_count_ / 10, kMaxPathLength)) {
      InterruptIfTableExpanded();
      path.push_back(current_bucket);
      LockSet lockset = lock_manager_.Lock(GetLockIndex(current_bucket));
      if (!buckets_[current_bucket].IsOccupied()) {
        return true;
      }
      auto pair = GetBuckets(buckets_[current_bucket].hash_);
      if (pair.first == current_bucket) {
        current_bucket = pair.second;
      } else {
        current_bucket = pair.first;
      }
    }
    return false;
  }

  CuckooPath FindCuckooPath(const TwoBuckets& start_buckets) {
    for (size_t i = 0; i < kFindPathIterations; ++i) {
      CuckooPath path;
      size_t start_bucket = start_buckets.first;
      if (rand() % 2) {
        start_bucket = start_buckets.second;
      }
      if (TryFindCuckooPathWithRandomWalk(start_bucket, path)) {
        return path;
      }
    }

    throw TableOvercrowded{};
  }

  void EvictElement(const TwoBuckets& buckets) {
    for (size_t i = 0; i < kEvictIterations; ++i) {
      auto path = FindCuckooPath(buckets);
      if (TryMoveHoleBackward(path)) {
        return;
      }
    }

    throw TableOvercrowded{};
  }

  void ExpandTable() {
    if (bucket_count_.load() != expected_bucket_count_) {
      return;
    }
    LockSet lockset = lock_manager_.LockAll();
    if (bucket_count_.load() != expected_bucket_count_) {
      return;
    }
    std::cout << "LoadFactor: " << GetLoadFactor() << std::endl;
    LOG_SIMPLE("EXPAND");
    size_t new_count = bucket_count_ * 2;
    buckets_.resize(new_count);
    for (int i = 0; i < bucket_count_; i++) {
      if (buckets_[i].IsOccupied()) {
        ElementHash hash = buckets_[i].hash_;
        if (GetPrimaryBucket(hash) == i &&
            GetPrimaryHash(hash) % new_count != i) {
          if (buckets_[GetPrimaryHash(hash) % new_count].IsOccupied()) {
            LOG_SIMPLE("FAIL");
            LOG_SIMPLE(GetPrimaryHash(hash));
          }
          buckets_[GetPrimaryHash(hash) % new_count] = buckets_[i];
          buckets_[i].Clear();
          continue;
        }
        if (GetAlternativeBucket(hash) == i &&
            GetAlternativeHash(hash) % new_count != i) {
          if (buckets_[GetAlternativeHash(hash) % new_count].IsOccupied()) {
            LOG_SIMPLE("FAIL");
            LOG_SIMPLE(GetPrimaryHash(hash));
          }
          buckets_[GetAlternativeHash(hash) % new_count] = buckets_[i];
          buckets_[i].Clear();
        }
      }
    }
    bucket_count_ = new_count;
    RememberBucketCount();
  }

 private:
  CuckooHasher<T, HashFunction> hasher_;

  mutable LockManager lock_manager_;

  tpcc::atomic<size_t> bucket_count_;
  tpcc::atomic<size_t> items_;
  CuckooBuckets buckets_;

  static thread_local size_t expected_bucket_count_;
};

template <typename T, typename HashFunction>
thread_local size_t CuckooHashSet<T, HashFunction>::expected_bucket_count_ = 0;

}  // namespace solutions
}  // namespace tpcc
