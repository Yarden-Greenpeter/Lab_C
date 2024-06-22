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

void execute_pipeline(char *cmd1[], char *cmd2[], char *input_file, char *output_file, int debugMode) {
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

        // Redirect input from file if specified
        if (input_file) {
            int input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1) {
                perror("open input file failed");
                exit(EXIT_FAILURE);
            }
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }

        // Execute command 1
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

        // Redirect output to file if specified
        if (output_file) {
            int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (output_fd == -1) {
                perror("open output file failed");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        // Execute command 2
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

            // Check for output redirection
            char *output_redirect = strchr(pipe_pos, '>');
            char *output_file = NULL;
            if (output_redirect) {
                *output_redirect = '\0';
                output_redirect++;
                output_file = strtok(output_redirect, " ");
            }

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

            // Execute the pipeline with redirection
            execute_pipeline(cmd1, cmd2, NULL, output_file, debugMode);
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

            // Check for input redirection
            char *input_redirect = strchr(input, '<');
            if (input_redirect) {
                *input_redirect = '\0';
                input_redirect++;
                char *input_file = strtok(input_redirect, " ");
                // Execute single command with input redirection
                execute(cmd, open(input_file, O_RDONLY), STDOUT_FILENO, debugMode);
            } else {
                // Execute single command without redirection
                execute(cmd, STDIN_FILENO, STDOUT_FILENO, debugMode);
            }
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

// void execute(char *command[], int input_fd, int output_fd, int debugMode) {
//     pid_t pid = fork();

//     if (pid == -1) {
//         perror("fork failed");
//         exit(EXIT_FAILURE);
//     }

//     if (pid == 0) {
//         // Child process
//         if (input_fd != STDIN_FILENO) {
//             dup2(input_fd, STDIN_FILENO);
//             close(input_fd);
//         }
//         if (output_fd != STDOUT_FILENO) {
//             dup2(output_fd, STDOUT_FILENO);
//             close(output_fd);
//         }

//         if (debugMode) {
//             fprintf(stderr, "PID: %d executing command: %s\n", getpid(), command[0]);
//         }

//         execvp(command[0], command);
//         perror("execvp failed");
//         exit(EXIT_FAILURE);
//     }
// }

// void execute_pipeline(char *cmd1[], char *cmd2[], int debugMode) {
//     int pipefd[2];
//     pid_t child1, child2;

//     if (pipe(pipefd) == -1) {
//         perror("pipe failed");
//         exit(EXIT_FAILURE);
//     }

//     // Fork first child for left-hand side command
//     if ((child1 = fork()) == -1) {
//         perror("fork failed");
//         exit(EXIT_FAILURE);
//     }

//     if (child1 == 0) {
//         // In the first child process
//         close(pipefd[0]); // Close read end of pipe
//         execute(cmd1, STDIN_FILENO, pipefd[1], debugMode);
//         close(pipefd[1]); // Close write end of pipe
//         exit(EXIT_SUCCESS);
//     }

//     // Fork second child for right-hand side command
//     if ((child2 = fork()) == -1) {
//         perror("fork failed");
//         exit(EXIT_FAILURE);
//     }

//     if (child2 == 0) {
//         // In the second child process
//         close(pipefd[1]); // Close write end of pipe
//         execute(cmd2, pipefd[0], STDOUT_FILENO, debugMode);
//         close(pipefd[0]); // Close read end of pipe
//         exit(EXIT_SUCCESS);
//     }

//     // Close pipe ends in parent process
//     close(pipefd[0]);
//     close(pipefd[1]);

//     // Wait for child processes to complete
//     waitpid(child1, NULL, 0);
//     waitpid(child2, NULL, 0);
// }

// int main(int argc, char **argv) {
//     char cwd[PATH_MAX];
//     char input[2048];
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

//         // Split the input by '|'
//         char *pipe_pos = strchr(input, '|');
//         if (pipe_pos) {
//             *pipe_pos = '\0';
//             pipe_pos++;

//             // Split commands into arguments
//             char *cmd1_str = input;
//             char *cmd2_str = pipe_pos;

//             // Tokenize command strings into argument arrays
//             char *cmd1[256];
//             char *cmd2[256];

//             int i = 0;
//             char *token = strtok(cmd1_str, " ");
//             while (token) {
//                 cmd1[i++] = token;
//                 token = strtok(NULL, " ");
//             }
//             cmd1[i] = NULL;

//             i = 0;
//             token = strtok(cmd2_str, " ");
//             while (token) {
//                 cmd2[i++] = token;
//                 token = strtok(NULL, " ");
//             }
//             cmd2[i] = NULL;

//             // Execute the pipeline
//             execute_pipeline(cmd1, cmd2, debugMode);
//         } else {
//             // No pipe, just a single command
//             char *cmd[256];
//             int i = 0;
//             char *token = strtok(input, " ");
//             while (token) {
//                 cmd[i++] = token;
//                 token = strtok(NULL, " ");
//             }
//             cmd[i] = NULL;

//             execute(cmd, STDIN_FILENO, STDOUT_FILENO, debugMode);
//         }
//     }

//     return 0;
// }
