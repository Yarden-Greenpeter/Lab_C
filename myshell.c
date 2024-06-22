#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "LineParser.h"

void execute(cmdLine *pCmdLine, int debugMode) {
    int runInBackground = pCmdLine->blocking == 0 ? 1 : 0;

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        _exit(EXIT_FAILURE);  // Exit abnormally if fork fails
    }

    // Child process
    if (pid == 0) {
        if (debugMode) {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
        }
        
        // 3--------------------------------------
        // Handle input redirection
        if (pCmdLine->inputRedirect) {
            int inputFd = open(pCmdLine->inputRedirect, O_RDONLY);
            if (inputFd == -1) {
                perror("open inputRedirect failed");
                _exit(EXIT_FAILURE);  // Exit abnormally if open fails
            }
            if (dup2(inputFd, STDIN_FILENO) == -1) {
                perror("dup2 inputRedirect failed");
                _exit(EXIT_FAILURE);  // Exit abnormally if dup2 fails
            }
            close(inputFd);  // Close the file descriptor after duplicating
        }

        // Handle output redirection
        if (pCmdLine->outputRedirect) {
            int outputFd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (outputFd == -1) {
                perror("open outputRedirect failed");
                _exit(EXIT_FAILURE);  // Exit abnormally if open fails
            }
            if (dup2(outputFd, STDOUT_FILENO) == -1) {
                perror("dup2 outputRedirect failed");
                _exit(EXIT_FAILURE);  // Exit abnormally if dup2 fails
            }
            close(outputFd);  // Close the file descriptor after duplicating
        }

        /* Input Redirection: If pCmdLine->inputRedirect is not NULL, open the file in read-only mode (O_RDONLY).
            Use dup2 to duplicate the file descriptor to STDIN_FILENO and then close the file descriptor.
        Output Redirection: If pCmdLine->outputRedirect is not NULL, open the file in write-only mode,
            creating it if it does not exist and truncating it if it does (O_WRONLY | O_CREAT | O_TRUNC). 
            Use dup2 to duplicate the file descriptor to STDOUT_FILENO and then close the file descriptor. */

        // ---------------------------

        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
            perror("execvp failed");
            _exit(EXIT_FAILURE);  // Exit abnormally if execvp fails
        }
    } else {
        // Parent process
        if (!runInBackground) {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid failed");
            }
        } else {
            if (debugMode) {
                fprintf(stderr, "Background process started: %s (PID: %d)\n", pCmdLine->arguments[0], pid);
            }
        }
    }
}

// 2-------------------------------------------------
void alarm_command(int pid) {
    if (kill(pid, SIGCONT) == -1) {
        perror("kill failed");
    } else {
        printf("Process with PID %d woken up successfully.\n", pid);
    }
}

void blast_command(int pid) {
    if (kill(pid, SIGKILL) == -1) {
        perror("kill failed");
    } else {
        printf("Process with PID %d terminated successfully.\n", pid);
    }
}

int main(int argc, char **argv) {
    char cwd[PATH_MAX];
    char input[2048];
    cmdLine *parsedCmdLine;
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

        parsedCmdLine = parseCmdLines(input);
        if (parsedCmdLine == NULL) {
            continue;
        }

        // 1c -------------------------
        // Check for the "cd" command
        if (strcmp(parsedCmdLine->arguments[0], "cd") == 0) {
            if (chdir(parsedCmdLine->arguments[1]) != 0) {
                fprintf(stderr, "cd failed: No such file or directory\n");
            }
            continue;
        }

        // Execute the command
        // execute(parsedCmdLine, debugMode);

        // 2------------------------------------------------- if its an alarm or blast - handle it. else, execute commend.
        if (strcmp(parsedCmdLine->arguments[0], "alarm") == 0 && parsedCmdLine->argCount == 2) {
            int pid = atoi(parsedCmdLine->arguments[1]);
            alarm_command(pid);
        } else if (strcmp(parsedCmdLine->arguments[0], "blast") == 0 && parsedCmdLine->argCount == 2) {
            int pid = atoi(parsedCmdLine->arguments[1]);
            blast_command(pid);
        } else {
            execute(parsedCmdLine, debugMode);
        }
        freeCmdLines(parsedCmdLine);
    }

    return 0;
}


// 2) The execv function requires the full path to the executable
//    because it does not search the directories listed in the PATH environment variable.
//    It only tries to execute the file specified by the given path.