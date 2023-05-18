#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
    int parent_fd[2], child_fd[2];
    char buf[1] = "a";

    if (pipe(parent_fd) < 0 || pipe(child_fd) < 0)
    {
        printf("pipe failed\n");
        exit(-1);
    }

    int pid = fork();
    if (pid == 0)
    {
        // child process
        close(parent_fd[1]);
        close(child_fd[0]);


        read(parent_fd[0], buf, 1);
        printf("%d: received ping\n", pid);
        write(child_fd[1], buf, 1);

        close(parent_fd[0]);
        close(child_fd[1]);
    }
    else
    {
        // parent process
        close(parent_fd[0]);
        close(child_fd[1]);

        write(parent_fd[1], buf, 1);
        read(child_fd[0], buf, 1);
        printf("%d: received pong\n", pid);

        close(parent_fd[1]);
        close(child_fd[0]);
    }

    exit(0);
}