#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int get_current_directory(char** current_dir, int argc, char* argv[]) {
  if (getcwd(*current_dir, PATH_MAX) == NULL) {        
   return -1;
  }
  if (argc >= 3) {
    if (chdir(argv[2]) < 0) {
      return -1;
    }
    *current_dir = strcat(*current_dir, "/");
    *current_dir = strcat(*current_dir, argv[2]);
  }
}

void swap(char** s1, char** s2) {
	char* temp;
  temp = *s1;
  *s1 = *s2;
  *s2 = temp;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    perror("specify port");
    exit(1);
  }
  const int port = atoi(argv[1]);
  char* current_dir = (char*)malloc(PATH_MAX * sizeof(char));
  char* new_dir = (char*)malloc(PATH_MAX * sizeof(char));
  if (get_current_directory(&current_dir, argc, argv) < 0) {
    perror("unable to enter this directory");
    exit(-1);
  }
  printf("Current directory: %s\n", current_dir);
  char* initial_dir = (char*)malloc(strlen(current_dir) * sizeof(char));
  initial_dir = strcpy(initial_dir, current_dir);

  int socketfd_server;
  if((socketfd_server = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("socket error");
    exit(-1);
  }  

  struct sockaddr_in addr;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = 0;
  addr.sin_family = AF_INET;
  if(bind(socketfd_server, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    exit(-1);
  }
  if(listen(socketfd_server, 1) == -1) {
    perror("listen error");
    exit(1);
  }  

  struct sockaddr_in client;
  int len = sizeof(client);
  int socketfd_client = accept(socketfd_server, (struct sockaddr*)&client, &len);

  const int buf_size = 100;
  const int ls_size = 4096;
  const int cmd_len = 5;
  char buf[buf_size];
  char cmd[cmd_len];
  char filename[buf_size];
  char dirname[buf_size];
  int file;
  struct stat filestat;
  while(1) {
    int status = 1;
    bzero(buf, buf_size);
    if (recv(socketfd_client, buf, buf_size, 0) <= 0) {
      perror("terminate listening");
      break;
    }
    sscanf(buf, "%s", cmd);
    if(!strcmp(cmd, "quit")) {
      perror("quit server");
      send(socketfd_client, &status, sizeof(int), 0);
      exit(0);
    }
    if (!strcmp(cmd, "put")) {
      sscanf(buf + 4, "%s", filename);
      printf("put %s\n", filename);
      int size;
      recv(socketfd_client, &size, sizeof(int), 0);
      if ((file = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666)) == -1) {
        perror("copy failed");
        status = 0;
      } else {
         if (size != 0) {
           char* f = malloc(size);
           recv(socketfd_client, f, size, 0);
           if (write(file, f, size) != size) {
             perror("copy error");
             status = 0;
         }
         free(f);
        }
        close(file);
      }
      send(socketfd_client, &status, sizeof(int), 0);
      continue;
    }
    if (!strcmp(cmd, "get")) {
      sscanf(buf + 4, "%s", filename);
      stat(filename, &filestat);
      len = filestat.st_size;
      printf("get %s\n", filename);
      if((file = open(filename, O_RDONLY)) == -1) {
        perror("failed to read file");
        status = 0;
      }
      send(socketfd_client, &len, sizeof(int), 0);
      if(len > 0) {
        sendfile(socketfd_client, file, NULL, len);
      }
      continue; 
    }
    if (!strcmp(cmd, "ls")) {
      printf("command: ls\n");
      int pipefd[2];
      if (pipe(pipefd) == -1) {
       perror("ls error");
       continue;
      }
      if (fork() == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], 1) < 0) {
          exit(-1);
        }
        close(pipefd[1]);
        execl("/bin/ls", "ls", (char*)0);
        exit(1);
      } else  {
        wait(NULL);
        close(pipefd[1]);
        char* receive_buffer = malloc(ls_size);
        len = read(pipefd[0], receive_buffer, ls_size);
        send(socketfd_client, &len, sizeof(int), 0);
        if (len > 0) {
          send(socketfd_client, receive_buffer, len, 0);
        }
        free(receive_buffer);
        continue;
      }
    } 
    if (!strcmp(cmd, "cd")) {
      sscanf(buf + 3, "%s", dirname);
      printf("command: cd %s\n", dirname);
      if (chdir(dirname) < 0) {
        perror("unable to enter this directory");
        status = 0;
      } else {
        getcwd(new_dir, PATH_MAX); 
        if (strlen(new_dir) < strlen(initial_dir) && strncmp(new_dir, initial_dir, strlen(new_dir)) == 0) {
          if (chdir(current_dir) < 0) {
            perror("security error. Quitting");
            exit(-1);
          }
          perror("permission denied");
          status = 0;
        } else {
          swap(&current_dir, &new_dir);
        }
      }
      printf("Current directory: %s\n", current_dir);
      send(socketfd_client, &status, sizeof(int), 0);
      continue;
    }
    printf("%s\n", cmd);
    perror("wrong command");
  }
  free(current_dir);
  return 0;
}
