#ifndef STRINGS_UTILS_H
#define STRINGS_UTILS_H

const size_t BLOCK_SIZE = 10;
const size_t INITIAL_LEN = 2;

#define CHK_pointer(p) do { \
  if (p == NULL) { \
    fputs("No memory left\n", stderr); \
    exit(-1); \
  } \
} while(0)

#define CHK_string do { \
  if (pos_in_word >= word_len) { \
    word_len *= 2; \
    dict[word] = (char*)realloc(dict[word], word_len * sizeof(char)); \
    if (dict[word] == NULL) { \
      fputs("No memory left\n", stderr); \
      break; \
    } \
  } \
} while(0)

#define NEXT_WORD do { \
  dict[word][pos_in_word] = '\0'; \
  word++; \
  word_len = INITIAL_LEN; \
  pos_in_word = 0; \
  if (word >= dict_size) { \
    dict = resize_dict(dict, &dict_size); \
  } \
} while(0)

char** resize_dict(char** p, size_t* size) {
  *size += BLOCK_SIZE;
  p = (char**)realloc(p, (*size) * sizeof(char*));
  CHK_pointer(p);
  for (int i = *size - BLOCK_SIZE; i < *size; i++) {
    p[i] = (char*) malloc(INITIAL_LEN * sizeof(char*));
    CHK_pointer(p[i]);
  }
  return p;
}

#endif
