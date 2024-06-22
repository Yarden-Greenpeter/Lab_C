#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t child1, child2;

    // Create a pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork the first child process (child1)
    if ((child1 = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {
        // In child1 process
        // Close the standard output
        close(STDOUT_FILENO);
        // Duplicate the write-end of the pipe using dup
        dup(pipefd[1]);
        // Close the file descriptor that was duplicated
        close(pipefd[1]);
        // Close the read end of the pipe
        close(pipefd[0]);
        // Execute "ls -l"
        execlp("ls", "ls", "-l", (char *)NULL);
        // If execlp fails
        perror("execlp");
        exit(EXIT_FAILURE);
    } 

    // In parent process
    // Close the write end of the pipe
    close(pipefd[1]);

    // Fork the second child process (child2)
    if ((child2 = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) {
        // In child2 process
        // Close the standard input
        close(STDIN_FILENO);
        // Duplicate the read-end of the pipe using dup
        dup(pipefd[0]);
        // Close the file descriptor that was duplicated
        close(pipefd[0]);
        // Execute "tail -n 2"
        execlp("tail", "tail", "-n", "2", (char *)NULL);
        // If execlp fails
        perror("execlp");
        exit(EXIT_FAILURE);
    } 

    // In parent process
    // Close the read end of the pipe
    close(pipefd[0]);

    // Wait for child1 to terminate
    waitpid(child1, NULL, 0);
    // Wait for child2 to terminate
    waitpid(child2, NULL, 0);

    return 0;
}
