#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int execute_strings(FILE* file) {
  int c = 'a';
  int follow = 0;
  char buffer[4];
  while (c != EOF) {
    c = getc(file);
    if (c == ' ' || c == '\t' || (c > 32 && c < 128)) {
        if (follow <= 3) {
            buffer[follow] = c;
        }
        if (follow == 3) {
            for (int i = 0; i < 4; i++) {
              putchar(buffer[i]);
            }
        }
        if (follow > 3) {
          putchar(c);
        }
        follow++;
    } else {
        if (c == '\n' && follow > 3) {
            putchar(c);
        }
        follow = 0;
    } 
  }
}

int main(int argc, char** argv) {
  if (argc == 1) {
    execute_strings(stdin);
    return 0; 
  }
  for (int i = 1; i < argc; ++i) {
    FILE* f;
    if((f = fopen(argv[i], "r")) == NULL) {
      printf("Failed to open file\n");
      continue;
    } 
    execute_strings(f);
    fclose(f);
  }
  return 0;
}