#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

#define CHK_pointer(p) do { \
  if (p == NULL) { \
    fputs("No memory left\n", stderr); \
    exit(-1); \
  } \
} while(0)

int execute_wc(const char* filename, long long int* lines, long long int* words, long long int* chars) {
    char* buffer;
    long long int len ;
    FILE* file = NULL;

	if (filename == NULL) {
		int c;
		long long int i = 0;
	    len = 16;
		buffer = (char*)malloc(len * sizeof(char));
		while ((c = getchar()) != EOF) {
			if (i >= len) {
				len *= 2;
				buffer = realloc(buffer, len * sizeof(char));
				CHK_pointer(buffer);
			}
            buffer[i] = c;
            i++;
		}
		len = i;
	} else {
		if((file = fopen(filename, "r")) == NULL) {
			perror("cannot open file");
			return -1;
		}
		struct stat st;
	    stat(filename, &st);
	    len = st.st_size;
	 	buffer = (char*)malloc(len * sizeof(char));
		CHK_pointer(buffer);
		if (fread(buffer, sizeof(char), len, file) != len*sizeof(char)) {
			perror("Failed to read file\n");
			return -1;
		}
	}
	
	
	*lines = 0;
	*words = 0;
	*chars = 0;
	wchar_t mbsymb;
	mbstate_t ps;
	long long int pos = 0;
	long long int word_start = 1;
	setlocale(LC_ALL, "");
	while (pos < len) {
		memset(&ps, 0, sizeof(ps));
		int shift = mbrtowc(&mbsymb, &buffer[pos], len - pos, &ps);
		if (shift < 0) {
			perror("Failed to read file");
			return -1;
		}
		(*chars)++;
		if (iswspace(mbsymb)) {
            if (mbsymb == L'\n') {
            	(*lines)++;
            }
            word_start = 1;
		} else {
			if (word_start) {
				(*words)++;
			}
			word_start = 0;
		}

		pos += shift;
	}
	if (file != NULL) {
		fclose(file);
	}
	return 0;
}

int main(int argc, char* argv[]) {
	long long int chars = 0;
	long long int words = 0;
	long long int lines = 0;
	long long int total_lines = 0;
	long long int total_words = 0;
	long long int total_chars = 0;
	if (argc == 1) {
		execute_wc(NULL, &lines, &words, &chars);
		printf("l: %lld w: %lld c: %lld\n", lines, words, chars);
		return 0;
	}
	
    for (int i = 1; i < argc; i++) {
		if (execute_wc(argv[i], &lines, &words, &chars) == 0) {
			total_lines += lines;
			total_words += words;
			total_chars += chars;
			printf("%lld %lld %lld %s\n", lines, words, chars, argv[i]);
		} else {
			printf("cannot open file %s\n", argv[i]);
			continue;
		}
	}
	if (argc > 2) {
		printf("%lld %lld %lld total\n", total_lines, total_words, total_chars);
	}

	return 0;
}