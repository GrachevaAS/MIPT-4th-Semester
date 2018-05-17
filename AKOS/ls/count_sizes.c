#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdlib.h>

#define CHK_pointer(p) do { \
  if (p == NULL) { \
    perror("Failed to read file info\n", stderr); \
    exit(-1); \
  } \
} while(0)

int max(unsigned int a, unsigned int b) {
    return a < b ? b : a;
}

unsigned int digits(unsigned int n) {
    unsigned int ans = 1;
    while (n >= 10) {
        ans++;
        n /= 10;
    }
    return ans;
}

int count_table_width(DIR* dir, const struct dirent *entry, unsigned int *table) {
    const char* filename = entry->d_name;
    static char time_line[15];
    int fd = dirfd(dir);
    if (fd < 0) {
        return -1;
    }
    struct stat st;
    if (fstatat(fd, filename, &st, 0)) {
        return -1;
    }

    struct passwd* p1 = getpwuid(st.st_uid);
    struct group* p2 = getgrgid(st.st_gid);
    int namelen;
    int grlen;
    if (p1 == NULL) {
        char str[8];
        sprintf(str, "%i", st.st_uid);
        namelen = strlen(str);
    } else { namelen = strlen(p1->pw_name); }
    if (p2 == NULL) {
        char str[8];
        sprintf(str, "%i", st.st_gid);
        grlen =  strlen(str);
    } else { grlen = strlen(p2->gr_name); }

    table[0] = max(table[0], digits(st.st_nlink));
    table[1] = max(table[1], namelen);
    table[2] = max(table[2], grlen);
    table[3] = max(table[3], digits(st.st_size));
    return 0;
}