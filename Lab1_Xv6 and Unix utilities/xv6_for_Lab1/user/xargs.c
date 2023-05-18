#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user.h"

void exec_block(const char* path, char** argv) {
    int pid = fork();
    if (pid == 0) {
        exec(path, argv); // echo 1 2 3
        exit(0);
    } else {
        wait(0);
    }
}

/**
 * Read a line from stdin.
 *  exit(1) if line too long
 *  return 0 if EOF
*/
char* readline() {
    static char buf[512];
    char *p = buf;
    int sz = 0;
    while(sz < 512 && read(0, p, 1)){
        if (*p == '\n') {
            *p = '\0';
            return buf;
        }
        p++;
        sz++;

        if (sz == 512) {
            printf("xargs: line too long\n");
            exit(0);
        }
    }

    return 0;
}

void _print_args(char **argv) {
    for (int i = 0; ; i++) {
        if (argv[i] == 0) break;
        printf("%s ", argv[i]);
    }
    printf("\n");
}

int main (int argc, const char* argv[]) {

    if (argc < 2) {
        printf("xargs: no command given\n");
        exit(1);
    }

    const char* cmd = argv[1];

    static char args_buf[MAXARG][512];
    int base_count = 0;
    for (int i = 1; i < argc; i++) {
        strcpy(args_buf[base_count], argv[i]);
        base_count += 1;
    }

    char* args[MAXARG+1];

    char* line = 0;
    while((line = readline()) != 0) {
        int count = base_count;
        
        char *q = line;
        for(;*q != '\0';q++){
            char* p = q;
            while(*p != ' ' && *p != '\0'){
                p++;
            }

            int is_end = 0;
            if (*p == '\0') is_end = 1;

            *p = '\0';
            strcpy(args_buf[count], q);
            count += 1;

            if (is_end) break;
            q = p;
        }

        for(int i = 0;i < count;i++){
            args[i] = args_buf[i];
        }
        args[count] = 0;

        // printf("xargs: ");
        // _print_args(args);
        exec_block(cmd, args);
    }

    return 0;
}