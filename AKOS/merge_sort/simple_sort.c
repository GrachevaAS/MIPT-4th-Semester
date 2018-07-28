#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int strcmp_new(const void *p1, const void *p2) {
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

int main(int argc, char ** argv) {
  	if (argc < 3) {
		fprintf(stderr,"Usage: %s processes filename\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char* filename = argv[2];
	//printf("%s\n", filename);
    FILE* fd = fopen(filename, "r");
    
	char* current_str = NULL;
	int c = 0;
	int count = 0;
	while ((c = getc(fd)) != EOF) {
		if (c == '\n') {
			count++;
		}
	}
	char** mem = malloc(count*sizeof(char*));	
	if (mem == NULL) {
		perror("memory error");
		exit(EXIT_FAILURE);
	}
	rewind(fd);
	for (int i = 0; i < count; i++) {
	  char* s = NULL;
	  size_t len = 0;
      if (getline(&s, &len, fd) < 0) {
      	free(s);
      	perror("error while reading lines");
      }
      mem[i] = s;
	}
	qsort(mem, count, sizeof(char*), strcmp_new);
	for (int i = 0; i < count; i++) {
      printf("%s", mem[i]);
	}
}