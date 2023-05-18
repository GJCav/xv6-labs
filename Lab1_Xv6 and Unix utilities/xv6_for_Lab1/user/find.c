#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"


const char* join(const char *base, const char *name) {
    static char buf[512];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, base);
    char *p = buf + strlen(buf);
    *p++ = '/';
    strcpy(p, name);
    return buf;
}

void find(const char* dir, const char* name) {
    int fd;
    struct stat st;

    if ((fd = open(dir, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", dir);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", dir);
        close(fd);
        return;
    }

    if (st.type != T_DIR) {
        return;
    }

    if (strlen(dir) + 1 + DIRSIZ + 1 > 512) {
        printf("find: path too long\n");
        close(fd);
        return;
    }

    struct dirent de;
    while(read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;

        const char *path = join(dir, de.name);
        // printf("try %s\n", path);

        struct stat child_st;
        if(stat(path, &child_st) < 0) {
            fprintf(2, "find: cannot stat %s\n", path);
            continue;
        }

        if (child_st.type == T_FILE) {
            if (strcmp(de.name, name) == 0) {
                printf("%s\n", path);
            }
        } else {
            char buf[512]; memset(buf, 0, sizeof(buf));
            strcpy(buf, path);
            find(buf, name);
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    // test structure
    // echo hello > file_aaa
    // mkdir dirA
    // echo hello > dirA/file_aaa
    // mkdir dirB
    // echo hello > dirB/file_aaa

    if (argc != 3) {
        printf("Usage: find <path> <name>\n");
        exit(-1);
    }

    char *root_path = argv[1];
    char *name = argv[2];


    find(root_path, name);

    exit(0);
}