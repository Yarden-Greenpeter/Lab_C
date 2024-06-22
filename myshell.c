#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void execute(char *command[], int input_fd, int output_fd, int debugMode) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        if (debugMode) {
            fprintf(stderr, "PID: %d executing command: %s\n", getpid(), command[0]);
        }

        execvp(command[0], command);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
}

void execute_pipeline(char *cmd1[], char *cmd2[], int debugMode) {
    int pipefd[2];
    pid_t child1, child2;

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // Fork first child for left-hand side command
    if ((child1 = fork()) == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) {
        // In the first child process
        close(pipefd[0]); // Close read end of pipe
        execute(cmd1, STDIN_FILENO, pipefd[1], debugMode);
        close(pipefd[1]); // Close write end of pipe
        exit(EXIT_SUCCESS);
    }

    // Fork second child for right-hand side command
    if ((child2 = fork()) == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) {
        // In the second child process
        close(pipefd[1]); // Close write end of pipe
        execute(cmd2, pipefd[0], STDOUT_FILENO, debugMode);
        close(pipefd[0]); // Close read end of pipe
        exit(EXIT_SUCCESS);
    }

    // Close pipe ends in parent process
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for child processes to complete
    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);
}

int main(int argc, char **argv) {
    char cwd[PATH_MAX];
    char input[2048];
    int debugMode = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debugMode = 1;
    }

    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s$ ", cwd);
        } else {
            perror("getcwd failed");
            continue;
        }

        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        input[strcspn(input, "\n")] = 0;

        // Exit the shell if the command is "quit"
        if (strcmp(input, "quit") == 0) {
            break;
        }

        // Split the input by '|'
        char *pipe_pos = strchr(input, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';
            pipe_pos++;

            // Split commands into arguments
            char *cmd1_str = input;
            char *cmd2_str = pipe_pos;

            // Tokenize command strings into argument arrays
            char *cmd1[256];
            char *cmd2[256];

            int i = 0;
            char *token = strtok(cmd1_str, " ");
            while (token) {
                cmd1[i++] = token;
                token = strtok(NULL, " ");
            }
            cmd1[i] = NULL;

            i = 0;
            token = strtok(cmd2_str, " ");
            while (token) {
                cmd2[i++] = token;
                token = strtok(NULL, " ");
            }
            cmd2[i] = NULL;

            // Execute the pipeline
            execute_pipeline(cmd1, cmd2, debugMode);
        } else {
            // No pipe, just a single command
            char *cmd[256];
            int i = 0;
            char *token = strtok(input, " ");
            while (token) {
                cmd[i++] = token;
                token = strtok(NULL, " ");
            }
            cmd[i] = NULL;

            execute(cmd, STDIN_FILENO, STDOUT_FILENO, debugMode);
        }
    }

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