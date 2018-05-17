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

struct table_entry {
    unsigned long long size;
    char* line;
    int isdir;
    char* path;
};

const char* get_rights(unsigned long mode) {
    static char full_rights[]= "rwxrwxrwx";
    static char str[10];
    str[9] = 0;
    int mask = 1 << 8;
    for (int i = 0; i < 9; i++) {
        str[i] = (mode & mask) ? full_rights[i] : '-';
        mask >>= 1;
    }
    return str;
}

struct table_entry* print_table_line(DIR* dir, const struct dirent* entry, unsigned int* table, const char* dirname) {
    const char* filename = entry->d_name;
    static char time_line[15];
    struct stat st;
    int fd = dirfd(dir);
    if (fd < 0) {
        return NULL;
    }
    if (fstatat(fd, filename, &st, 0)) {
        return NULL;
    }
    char mode_pref;
    if (S_ISDIR(st.st_mode)) { 
        mode_pref = 'd'; 
    } else if (S_ISCHR(st.st_mode)) {
        mode_pref = 'c'; 
    } else if (S_ISBLK(st.st_mode)) {
        mode_pref = 'b'; 
    } else if (S_ISLNK(st.st_mode)) {
        mode_pref = 'l';
    } else {
        mode_pref = '-';
    }
    strftime(time_line, 15, "%b %d %H:%M", localtime(&st.st_ctime));

    int pathlen = strlen(dirname) + strlen(filename) + 2;
    char* path = malloc(pathlen * sizeof(char));
    if (path == 0) {
        return NULL;
    }
    sprintf(path, "%s/%s", dirname, filename);

    struct table_entry* result = (struct table_entry*)malloc(sizeof(struct table_entry));
    if (result == 0) {
        return NULL;
    }
    result->size = st.st_size;
    int line_len = 32 + strlen(filename);
    for (int i = 0; i < 5; i++) {
        line_len += table[i];
    }
    result->line = (char*) malloc(sizeof(char) * line_len);
    if (result->line == 0) {
        return NULL;
    }
    struct passwd* p1 = getpwuid(st.st_uid);
    struct group* p2 = getgrgid(st.st_gid);
    char* name;
    char* gr;
    char namestr[8];
    char grstr[8];
    if (p1 == NULL) {
        sprintf(namestr, "%i", st.st_uid);
        name = &namestr[0];
    } else { name = p1->pw_name; }
    if (p2 == NULL) {
        sprintf(grstr, "%i", st.st_gid);
        gr =  &grstr[0];
    } else { gr = p2->gr_name; }

    sprintf(result->line, "%c%s %*ld %*s %*s %*ld %*s %s\n", 
        mode_pref, get_rights(st.st_mode), table[0], st.st_nlink, 
        table[1], name, table[2], gr, table[3], st.st_size, table[4], time_line, filename);
    result->path = path;
    result->isdir = 0;
    if (S_ISDIR(st.st_mode) && strcmp(filename, ".") && strcmp(filename, "..")) {
        result->isdir = 1;
    }

    return result;
}