#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

int strcmp_new(const void *p1, const void *p2) {
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

int main() {
  int c = '\0';
  int lexem_once = 0;
  size_t dict_size = BLOCK_SIZE;
  char** dict = (char**) malloc(dict_size * sizeof(char*));
  CHK_pointer(dict);
  for (int i = 0; i < BLOCK_SIZE; i++) {
    dict[i] = (char*) malloc(INITIAL_LEN * sizeof(char));
    CHK_pointer(dict[i]);
  }
  size_t word = 0;
  size_t pos_in_word = 0;
  size_t word_len = INITIAL_LEN;
  char quotes = 0;
  int quotes_type = '\0';

  while (c != EOF) {
    c = getchar();
    if (c == EOF) { continue; }

    if (c != '|' && c != '&' && c != ';') {
      lexem_once = 0;
    }

    if (quotes && c == quotes_type) {
      quotes = 0;
      c = getchar();
      if (isspace(c) || c == '|' || c == '&') {
        NEXT_WORD;
      }
      ungetc(c, stdin);
      continue;
    }

    if (quotes) {
      dict[word][pos_in_word] = c;
      pos_in_word++;
      CHK_string;
      continue;
    }

    if (lexem_once && c == lexem_once) {
      word--;
      pos_in_word = 1;
      dict[word][pos_in_word] = c;
      pos_in_word++;
      CHK_string;
      NEXT_WORD;
      lexem_once = 0;
      continue;
    }

    if (c == '|' || c == '&' || c == ';') {
      if (pos_in_word != 0) {
        NEXT_WORD;
      }
      dict[word][pos_in_word] = c;
      pos_in_word++;
      NEXT_WORD;
      lexem_once = c;
      continue;
    }

    if (c == '\"' || c == '\'') {
      quotes = 1;
      quotes_type = c;
      continue;
    }

    if (!isspace(c)) {
      dict[word][pos_in_word] = c;
      pos_in_word++;
      CHK_string;
    } else if (pos_in_word != 0) {
      NEXT_WORD;
    }
  }

  if (quotes) {
    fputs("quotes error/нет баланса кавычек\n", stderr);
    return 0;
  }
  if (pos_in_word != 0) {
    dict[word][pos_in_word] = '\0';
    word++;
  }

  qsort(dict, word, sizeof(char*), strcmp_new);
  for (int i = 0; i < word; i++) {
    printf("\"%s\"\n", dict[i]);
  }

  for (int i = 0; i < dict_size; i++) {
    free(dict[i]);
  }
  free(dict);
  return 0;
}
