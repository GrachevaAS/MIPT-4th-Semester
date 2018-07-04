#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc,char *argv[]) {
  if (argc < 3) {
    perror("specify host and port");
    exit(1);
  }
  char* host = argv[1];
  const int port = atoi(argv[2]);    

  int socketfd;
  if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }  

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(host); 
  printf("connecting to host %s %i\n", host, port); 
  if(connect(socketfd,(struct sockaddr*)&server, sizeof(server)) == -1) {
    perror("connection error");
    exit(-1);
  }

  size_t line_size = 100;
  const int cmd_len = 5; 
  char filename[line_size];
  char dirname[line_size];
  char* cmd_line = (char*)malloc(line_size * sizeof(char));
  int status = 1;
  struct stat filestat;
  int file;
  int size;

  char* current_dir = (char*)malloc(PATH_MAX * sizeof(char));
  getcwd(current_dir, PATH_MAX);
  printf("Current directory: %s\n", current_dir);

  while(1) {
    getline(&cmd_line, &line_size, stdin);
    if (strncmp(cmd_line, "quit", 4) == 0) {
      send(socketfd, cmd_line, cmd_len, 0);
      recv(socketfd, &status, sizeof(int), 0);
      if(status) { 
        printf("connection closed\n");
        exit(0);
      }
      printf("failed to close connection\n");
    }
    if (strncmp(cmd_line, "put", 3) == 0) {
      sscanf(cmd_line + 4, "%s", filename);
      if((file = open(filename, O_RDONLY)) == -1) {
        printf("can not open file to send\n");
        continue;
      }
      send(socketfd, cmd_line, strlen(cmd_line), 0);
      stat(filename, &filestat);
      size = filestat.st_size;
      send(socketfd, &size, sizeof(int), 0);
      if (size != 0) {
        sendfile(socketfd, file, NULL, size);
      }
      recv(socketfd, &status, sizeof(int), 0);
      if(status) { printf("successfully sent file\n");
    } else { printf("failed to send file\n"); }
    continue;
    }
    if (strncmp(cmd_line, "get", 3) == 0) {
      sscanf(cmd_line + 4, "%s", filename);
      send(socketfd, cmd_line, strlen(cmd_line), 0);
      recv(socketfd, &size, sizeof(int), 0);
      if(!size) {
        printf("no such file or the file is empty\n");
        continue;
      }
      char* receive_buffer = malloc(size);
      recv(socketfd, receive_buffer, size, 0);
      if ((file = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666)) == -1) {
        perror("failed to create a copy");
        free(receive_buffer);
        continue;
      }
      write(file, receive_buffer, size);
      close(file);
      free(receive_buffer);
      printf("successfully got the file\n");
      continue;
    }
    if (strncmp(cmd_line, "ls", 2) == 0) {
      send(socketfd, cmd_line, strlen(cmd_line), 0);
      recv(socketfd, &size, sizeof(int), 0);
      if (size > 0) {
        char* receive_buffer = malloc(size);
        recv(socketfd, receive_buffer, size, 0);
        write(1, receive_buffer, size);
        free(receive_buffer);
      } else {
        printf("nothing to list\n");
      }
      continue;
    }
    if (strncmp(cmd_line, "cd", 2) == 0) {
      send(socketfd, cmd_line, strlen(cmd_line), 0);
      recv(socketfd, &status, sizeof(int), 0);
      if(status) { printf("successfully changed directory\n"); 
    } else { printf("fail: no such directory or permission denied\n"); }
    continue;
    }
    if (strncmp(cmd_line, "!cd", 3) == 0) {
      printf("CD\n");
      sscanf(cmd_line + 4, "%s", dirname);
      if (chdir(dirname) < 0) {
        perror("failed to change directory");
      }
      getcwd(current_dir, PATH_MAX);
      printf("Current directory: %s\n", current_dir);
      continue;
    }
    if (strncmp(cmd_line, "!ls", 3) == 0) {
      if (fork() == 0) {
        execl("/bin/ls", "ls", (char*)0);
        exit(0);
      } else {
        wait(NULL);
      }
      continue;
    }
  }
  free(cmd_line);
}