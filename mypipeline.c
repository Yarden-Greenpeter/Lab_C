#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t child1, child2;

    fprintf(stderr, "(parent_process>forking…)\n");
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if ((child1 = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", child1);

    if (child1 == 0) {
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");

        close(STDOUT_FILENO);
        dup(pipefd[1]);
        close(pipefd[1]);
        close(pipefd[0]);

        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");

        execlp("ls", "ls", "-l", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } 

    fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
    close(pipefd[1]);

    if ((child2 = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", child2);

    if (child2 == 0) {
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");

        close(STDIN_FILENO);
        dup(pipefd[0]);
        close(pipefd[0]);

        fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");

        execlp("tail", "tail", "-n", "2", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } 

    fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
    close(pipefd[0]);

    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    fprintf(stderr, "(parent_process>exiting…)\n");

    return 0;
}

/*
    Task 1:
        1. When the parent process does not close the write end of the pipe,
            the second child process (child2) that executes tail -n 2 will not terminate properly.
            This is because tail -n 2 expects the end of the input (EOF) to know when to stop reading from the pipe
        2. tail -n 2 continue to wait for more data and will not receive an indication that no more data is coming through the pipe.
        3. the progeam end before the tasks were executed by the children.
*/
