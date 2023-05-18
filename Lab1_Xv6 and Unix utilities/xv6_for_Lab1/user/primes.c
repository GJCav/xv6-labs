#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define EOP 1 // end of primes

void filter(int base, int read_fd) {
    printf("prime %d\n", base);

    int write_fd = 0;

    int v;
    char buf[1];

    while(read(read_fd, buf, 1)) {
        v = buf[0];
        
        if (v == EOP) {
            buf[0] = EOP;
            if (write_fd != 0) {
                write(write_fd, buf, 1);
                int child_status = 0;
                wait(&child_status);
            }
            break;
        } else if (v % base != 0) {
            if (write_fd == 0) {
                int fd[2];
                if (pipe(fd) < 0) {
                    printf("pipe failed\n");
                    exit(-1);
                }

                int pid = fork();
                if (pid == 0) {
                    // child process
                    close(fd[1]);
                    filter(v, fd[0]);
                    return;
                } else {
                    // parent process
                    close(fd[0]);
                    write_fd = fd[1];
                }
            }

            write(write_fd, buf, 1);
        } else {
            // drop
        }
    }

    close(read_fd);
}

int main(int argc, char *argv[]) {
    int fd[2];
    char buf[1];

    if (pipe(fd) < 0) {
        printf("pipe failed\n");
        exit(-1);
    }

    int pid = fork();
    if (pid == 0) {
        // child process
        close(fd[1]);
        filter(2, fd[0]);
    } else {
        // parent process
        close(fd[0]);
        for(int i = 3;i <= 35;i += 1) {
            buf[0] = i;
            write(fd[1], buf, 1);
        }
        buf[0] = EOP;
        write(fd[1], buf, 1);

        int status = 0;
        wait(&status);
    }
    exit(0);
}