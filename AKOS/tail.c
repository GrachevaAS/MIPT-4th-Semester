#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHK_pointer(p) do { \
  if (p == NULL) { \
    fputs("No memory left\n", stderr); \
    exit(-1); \
  } \
} while(0)

void execute_tail(FILE* file) {
	char c;
	int current_size = 512;
	int strings = 0;
	int to_print = 0;
	int newline = 0;
	fseek(file, 0, SEEK_END);
	int len = ftell(file);
	char* buffer = malloc(current_size * sizeof(char));
	CHK_pointer(buffer);
	fseek(file, -1, SEEK_END);
	if (fgetc(file) == '\n') {
		strings--;
	} else {
		newline = 1;
	}
	for (int i = 0; i < len; i++) {
		if (i >= current_size) {
			current_size *= 2;
            buffer = realloc(buffer, current_size * sizeof(char));
            CHK_pointer(buffer);
		}
		fseek(file, -(i + 1), SEEK_END);
		c = fgetc(file);
		if (c == '\n') {
			if (++strings == 10) {
				break;
			}
		}
        buffer[i] = c;
        to_print++;
	}
	for (int i = 0; i < to_print; i++) {
		putchar(buffer[to_print - 1 - i]);
	}
	if (newline) {
		putchar('\n');
	}
}
void execute_tail_stdin() {
	char* buffer[10];
	for(int i = 0; i < 10; i++) {
		buffer[i] = NULL;
	}
	int pos = 0;
	size_t len = 0;
	char* s = NULL;
	while (getline(&s, &len, stdin) >= 0) {
		if (buffer[pos] != NULL) {
			free(buffer[pos]);
		}
		buffer[pos] = s;
		pos++;
		pos %= 10;
		s = NULL;
	}
	if (s != NULL) {
		free(s);
	}
	for (int i = 0; i < 10; i++) {
		if (buffer[pos % 10] != NULL) {
			printf("%s", buffer[pos % 10]);
		}
		pos++;
	}
	for (int i = 0; i < 10; i++) {
		if (buffer[i] != NULL) {
			free(buffer[i]);
		}
	}
}

int main(int argc, char** argv) {
	if (argc == 1) {
		execute_tail_stdin();
		return 0;
	}
	for (int i = 1; i < argc; ++i) {
		FILE* f;
		if((f = fopen(argv[i], "r")) == NULL) {
			printf("cannot open file %s\n", argv[i]);
			continue;
		} 
		if (argc > 2) {
			printf("==> %s <==\n", argv[i]);
		}
		execute_tail(f);
		fclose(f);
	}
	return 0;
}