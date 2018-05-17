#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>

void exec_program(const char* program, int current[2], int next[2], char is_start, char is_finish) {
    pid_t id = fork();
    if (id == -1) {
        perror("fork");
        exit(-1);
    }
    if (id == 0) {
        if (!is_start) {
            if (close(current[1]) < 0) {
                perror("close");
            }
            if (dup2(current[0], 0) < 0) { 
                perror("read"); 
            }
        }
        if (!is_finish) {
            if (close(next[0]) < 0) {
                perror("close"); 
            }
            if (dup2(next[1], 1) < 0) { 
                perror("write"); 
            }
        }
        execlp(program, program, NULL);
        perror(program);
        exit(-1);
    } else {
        if (!is_start) { close(current[0]); }
        close (current[1]);
    }
} 

int main(int argc, char* argv[]) {
    int pipes_d[2][2];
    pipe(pipes_d[0]);
    for (int i = 1; i < argc; i++) {
        if (i != argc - 1) {
            pipe(pipes_d[i % 2]);
        }
        exec_program(argv[i], pipes_d[1 - i % 2], pipes_d[i % 2], i == 1, i == argc - 1);
    }
    for (int i = 1; i < argc; i++) {
        wait(NULL);
    }
}