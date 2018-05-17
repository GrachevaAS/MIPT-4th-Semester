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

#define EBADF_CHECK do { \
    if (errno == EBADF) { \
        printf("Invalid descriptor in %s\n", dirname); \
        return -1; \
    } \
} while(0)

struct table_entry {
    unsigned long long size;
    char* line;
    int isdir;
    char* path;
};

int compare_by_size(const void* line1, const void* line2) {
    struct table_entry* L1 = *(struct table_entry* const*)line1;
    struct table_entry* L2 = *(struct table_entry* const*)line2;
    return L1->size < L2->size;
}

struct table_entry* print_table_line(DIR* dir, const struct dirent* entry, unsigned int* table, const char* dirname);
int count_table_width(DIR *dir, const struct dirent *entry, unsigned int *table);

int print_dir(const char* dirname) {
    if (strcmp(dirname, ".")) {
        printf("%s:\n", dirname);
    }
    DIR* dir;
    struct dirent* entry;
    dir = opendir(dirname);
    if (dir == NULL) {
        printf("Failed to open directory %s\n", dirname);
        return -1;
    }
    
    // сначала посчитаем, сколько места оставить для колонок, заполнив массив maximal_width
    const int cols = 5;
    unsigned int maximal_width[cols];
    for (int i = 0; i < cols; i++) {
        maximal_width[i] = 0;
    }
    maximal_width[4] = 12; //"May 30 11:11"
    int entry_count = 0;
    errno = 0;
    entry = readdir(dir);
    EBADF_CHECK;
    while (entry != NULL) {
        if (count_table_width(dir, entry, maximal_width) >= 0) {
            entry_count++;
        }
        errno = 0;
        entry = readdir(dir);
        EBADF_CHECK;
    }
    rewinddir(dir);
    struct table_entry* entry_list[entry_count];
    entry_count = 0;
    errno = 0;
    entry = readdir(dir);
    EBADF_CHECK;
    while (entry != NULL) {
        if ((entry_list[entry_count] = print_table_line(dir, entry, maximal_width, dirname)) != NULL) {
            entry_count++;
        } 
        errno = 0;
        entry = readdir(dir);
        EBADF_CHECK;
    }
    qsort(entry_list, entry_count, sizeof(struct table_entry*), compare_by_size);
    for (int i = 0; i < entry_count; i++) {
        if (entry_list[i] != NULL) {
            printf("%s", entry_list[i]->line);
        }
    }
    for (int i = 0; i < entry_count; i++) {
        if (entry_list[i] != NULL && entry_list[i]->isdir) {
            printf("\n");
            print_dir(entry_list[i]->path);
        }
    }
    
    for (int i = 0; i < entry_count; i++) {
        if (entry_list[i] != NULL) {
            if (entry_list[i]->line != NULL) {
                free(entry_list[i]->line);
            }
            if (entry_list[i]->path != NULL) {
                free(entry_list[i]->path);
            }
            free(entry_list[i]);
        }
    }
    if (closedir(dir) < 0) {
        printf("Failed to close directory %s\n", dirname);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        print_dir(".");
    }

    for (int i = 1; i < argc; i++) {
        print_dir(argv[i]);
    }

    return 0;
}
