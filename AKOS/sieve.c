#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <math.h>

long long limit;
unsigned char* sieve;

unsigned char masks[8];

void* mark_composites(void* arg) {
    long long p = *(long long *)arg;
    for(long long i = p * p ; i <= limit; i += p) {
        sieve[i] = 1;
    }
}

char checkifprime(long long x) {
    for (long long i = 2; i < x / 2; i++) {
        if (x % i == 0) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, const char * argv[]) {
    for (int i = 7; i >= 0; i--) {
        masks[i] = 1 << i;
    }
    if(argc == 1) {
       perror("specify thread number");
       exit(1);
    }
    if (argc == 2) {
        limit = UINT_MAX;
    } else {
        limit = atoll(argv[2]);
    }
    long long size = limit;
    if (limit < 256) {
        size = 256;
    }
    long long threads_count = atoi(argv[1]);
    sieve = (unsigned char*)malloc((size + 1) * sizeof(unsigned char));
    if (sieve == NULL) {
        perror("Allocation problem");
        exit(-1);
    }
    long long low_limit = (long long)round(sqrt(limit)) + 1;
    for (long long i = 0; i <= size; i++) {
        sieve[i] = 0;
    }
    long long initial_check = 4;
    int count = 0;
    do {
        if (!checkifprime(initial_check)) {
            sieve[initial_check] = 1;
        } else {
            count++;
        }
        initial_check++;
    } while (count < threads_count);
    long long checked = initial_check - 1;
    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * threads_count);
    long long* run = (long long *) malloc(sizeof(long long) * threads_count);
    long long current_pos = 2;
    while (current_pos <= low_limit) {
        long long t = 0;
        long long i = current_pos;
        while (i <= low_limit && i <= checked && t < threads_count) {
            if (!sieve[i]) {
                run[t] = i;
                pthread_create(threads + t, NULL, mark_composites, run + t);
                t++;
            }
            i++;
        }
        void *p;
        for(long long j = 0; j < t; j++) {
            pthread_join(threads[j], &p);
        }
        for(long long j = current_pos; j < i; j++) {
            if (!sieve[j]) {
                printf("%lld\n", j);
            }
        }
        current_pos = i;
        checked = current_pos * current_pos;
    }

    for (long long j = current_pos; j <= limit; j++) {
        if (!sieve[j]) {
            printf("%lld\n", j);
        }
    }
    free(sieve);
    free(threads);
    free(run);
    return 0;
}
