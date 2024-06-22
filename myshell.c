#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void execute_pipeline(int debugMode) {
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
        if (debugMode) {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: ls -l\n");
        }

        // Close the standard output
        close(STDOUT_FILENO);
        // Duplicate the write-end of the pipe using dup
        dup(pipefd[1]);
        // Close the file descriptor that was duplicated
        close(pipefd[1]);
        // Close the read end of the pipe
        close(pipefd[0]);

        // Arguments for "ls -l"
        char *ls_args[] = {"ls", "-l", NULL};

        // Execute "ls -l"
        execvp(ls_args[0], ls_args);

        // If execvp fails
        perror("execvp ls -l failed");
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
        if (debugMode) {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: tail -n 2\n");
        }

        // Close the standard input
        close(STDIN_FILENO);
        // Duplicate the read-end of the pipe using dup
        dup(pipefd[0]);
        // Close the file descriptor that was duplicated
        close(pipefd[0]);

        // Arguments for "tail -n 2"
        char *tail_args[] = {"tail", "-n", "2", NULL};

        // Execute "tail -n 2"
        execvp(tail_args[0], tail_args);

        // If execvp fails
        perror("execvp tail -n 2 failed");
        exit(EXIT_FAILURE);
    }

    // In parent process
    // Close the read end of the pipe
    close(pipefd[0]);

    // Wait for child1 to terminate
    waitpid(child1, NULL, 0);
    // Wait for child2 to terminate
    waitpid(child2, NULL, 0);
}

int main(int argc, char **argv) {
    int debugMode = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debugMode = 1;
    }

    // Directly execute the pipeline without reading input
    execute_pipeline(debugMode);

    return 0;
}

// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <limits.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include "LineParser.h"

// void execute(cmdLine *pCmdLine, int debugMode) {
//     int runInBackground = pCmdLine->blocking == 0 ? 1 : 0;

//     pid_t pid = fork();

//     if (pid == -1) {
//         perror("fork failed");
//         _exit(EXIT_FAILURE);  // Exit abnormally if fork fails
//     }

//     // Child process
//     if (pid == 0) {
//         if (debugMode) {
//             fprintf(stderr, "PID: %d\n", getpid());
//             fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
//         }
        
//         // 3--------------------------------------
//         // Handle input redirection
//         if (pCmdLine->inputRedirect) {
//             int inputFd = open(pCmdLine->inputRedirect, O_RDONLY);
//             if (inputFd == -1) {
//                 perror("open inputRedirect failed");
//                 _exit(EXIT_FAILURE);  // Exit abnormally if open fails
//             }
//             if (dup2(inputFd, STDIN_FILENO) == -1) {
//                 perror("dup2 inputRedirect failed");
//                 _exit(EXIT_FAILURE);  // Exit abnormally if dup2 fails
//             }
//             close(inputFd);  // Close the file descriptor after duplicating
//         }

//         // Handle output redirection
//         if (pCmdLine->outputRedirect) {
//             int outputFd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
//             if (outputFd == -1) {
//                 perror("open outputRedirect failed");
//                 _exit(EXIT_FAILURE);  // Exit abnormally if open fails
//             }
//             if (dup2(outputFd, STDOUT_FILENO) == -1) {
//                 perror("dup2 outputRedirect failed");
//                 _exit(EXIT_FAILURE);  // Exit abnormally if dup2 fails
//             }
//             close(outputFd);  // Close the file descriptor after duplicating
//         }


//         if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
//             perror("execvp failed");
//             _exit(EXIT_FAILURE);  // Exit abnormally if execvp fails
//         }
//     } else {
//         // Parent process
//         if (!runInBackground) {
//             int status;
//             if (waitpid(pid, &status, 0) == -1) {
//                 perror("waitpid failed");
//             }
//         } else {
//             if (debugMode) {
//                 fprintf(stderr, "Background process started: %s (PID: %d)\n", pCmdLine->arguments[0], pid);
//             }
//         }
//     }
// }

// void alarm_command(int pid) {
//     if (kill(pid, SIGCONT) == -1) {
//         perror("kill failed");
//     } else {
//         printf("Process with PID %d woken up successfully.\n", pid);
//     }
// }

// void blast_command(int pid) {
//     if (kill(pid, SIGKILL) == -1) {
//         perror("kill failed");
//     } else {
//         printf("Process with PID %d terminated successfully.\n", pid);
//     }
// }

// int main(int argc, char **argv) {
//     char cwd[PATH_MAX];
//     char input[2048];
//     cmdLine *parsedCmdLine;
//     int debugMode = 0;

//     if (argc > 1 && strcmp(argv[1], "-d") == 0) {
//         debugMode = 1;
//     }

//     while (1) {
//         if (getcwd(cwd, sizeof(cwd)) != NULL) {
//             printf("%s$ ", cwd);
//         } else {
//             perror("getcwd failed");
//             continue;
//         }

//         if (fgets(input, sizeof(input), stdin) == NULL) {
//             perror("fgets failed");
//             continue;
//         }

//         input[strcspn(input, "\n")] = 0;

//         // Exit the shell if the command is "quit"
//         if (strcmp(input, "quit") == 0) {
//             break;
//         }

//         parsedCmdLine = parseCmdLines(input);
//         if (parsedCmdLine == NULL) {
//             continue;
//         }

//         if (strcmp(parsedCmdLine->arguments[0], "cd") == 0) {
//             if (chdir(parsedCmdLine->arguments[1]) != 0) {
//                 fprintf(stderr, "cd failed: No such file or directory\n");
//             }
//             continue;
//         }

//         if (strcmp(parsedCmdLine->arguments[0], "alarm") == 0 && parsedCmdLine->argCount == 2) {
//             int pid = atoi(parsedCmdLine->arguments[1]);
//             alarm_command(pid);
//         } else if (strcmp(parsedCmdLine->arguments[0], "blast") == 0 && parsedCmdLine->argCount == 2) {
//             int pid = atoi(parsedCmdLine->arguments[1]);
//             blast_command(pid);
//         } else {
//             execute(parsedCmdLine, debugMode);
//         }
//         freeCmdLines(parsedCmdLine);
//     }

//     return 0;
// }