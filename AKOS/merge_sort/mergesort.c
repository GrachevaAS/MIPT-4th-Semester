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

void sort_piece(char** mem, int left, int right) {
  qsort(mem + left, right - left, sizeof(char*), strcmp_new);
}

void merge_two(char** const first,
	           char** const second,
	           char** const end, 
	           char** const target) {
	char** it1 = first;
	char** it2 = second;
	for (char** it = target; it < target + (end - first); it++) {
		if (it1 != second && (it2 == end || strcmp(*it1, *it2) < 0)) {
			*it = *it1;
			it1++;
		} else {
			*it = *it2;
			it2++;
		}
	}
	char** i = target;
	char** j = first;
	for (; j < end; i++, j++) {
		*j = *i;
	}
}
int min(int x, int y) {
	if (y < x) {
		return y;
	}
	return x;
}
int main(int argc, char ** argv) {
  	if (argc < 3) {
		fprintf(stderr,"Usage: %s processes filename\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	int proc_count = atoi(argv[1]);
	char* filename = argv[2];
    FILE* fd = fopen(filename, "r");
	int c = 0;
	int count = 0;
	long long len = 0;
	
	while ((c = getc(fd)) != EOF) {
		if (c == '\n') {
			count++;
		}
		len++;
	}

	char** mem = (char**)mmap(NULL, count * sizeof(char*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	char* strings = (char*)mmap(NULL, len + count, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (strings == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	rewind(fd);
	long long start = 0;
	long long pos = 0;
	int i = 0;
	while ((c = getc(fd)) != EOF) {
		strings[pos] = (char)c;
		pos++;
		if (c == '\n') {
			mem[i] = &strings[start];
			i++;
			strings[pos] = '\0';
			pos++;
			start = pos;
		}
	}
    int piece_len = count / proc_count;
    int left = 0;
    int right = 0;
    for (int i = 0; i < proc_count - 1; i++) {
        right = left + piece_len;
        int pid = fork();
        if (pid == -1) {
        	perror("filed to fork");
        	exit(EXIT_FAILURE);
        } 
        if (pid == 0) {
        	sort_piece(mem, left, right);
        	exit(0);
        } else {
        	left = right;
        	right += piece_len;
        }
    }
    right = count;
    int pid = fork();
    if (pid == -1) {
    	perror("failed to fork");
    	exit(EXIT_FAILURE);
    } 
    if (pid == 0) {
    	sort_piece(mem, left, right);
    	exit(0);
    }
    for (int i = 0; i < proc_count; i++) {
    	wait(NULL);
    }
	char** ans = malloc(count * sizeof(char*));
	if (ans < 0) {
		perror("memory error");
		exit(-1);
	}
  
	int step = 1;
	int it = proc_count;
	while (step < proc_count) {
		int pos = 0;
		int step_len = step * piece_len;
		for (int i = 0; i < it / 2; i++) {
			int end = pos + 2 * step_len;
			if (i == it / 2 - 1 && it % 2 == 0) {
				end = count;
			}
			merge_two(mem + pos, mem + pos + step_len, mem + end, ans + pos);
			pos += 2 * step_len;
		}
		it = it / 2 + it % 2;
		step *= 2;
	}
	for (int i = 0; i < count; i++) {
      printf("%s", mem[i]);
	}    

	if (munmap(mem, count) == -1) {
		perror("unmmap");
		exit(EXIT_FAILURE);
	}
	if (munmap(strings, count) == -1) {
		perror("unmmap");
		exit(EXIT_FAILURE);
	} 
}