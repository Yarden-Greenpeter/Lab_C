#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0

typedef struct cmdLine {
    char **arguments;
    int argCount;
    int blocking;
} cmdLine;

typedef struct process {
    cmdLine *cmd;
    pid_t pid;
    int status;
    struct process *next;
} process;

process *process_list = NULL;

//Process managment functions
void addProcess(process **process_list, cmdLine *cmd, pid_t pid) {
    process *new_process = (process *)malloc(sizeof(process));
    new_process->cmd = cmd;
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = *process_list;
    *process_list = new_process;
}

void updateProcessStatus(process *process_list, int pid, int status) {
    process *curr_process = process_list;
    while (curr_process != NULL) {
        if (curr_process->pid == pid) {
            curr_process->status = status;
            return;
        }
        curr_process = curr_process->next;
    }
}

void updateProcessList(process **process_list) {
    process *curr_process = *process_list;
    int status;
    pid_t result;
    
    while (curr_process != NULL) {
        result = waitpid(curr_process->pid, &status, WNOHANG);
        if (result == -1) {
            perror("waitpid failed");
            return;
        }
        if (result > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                curr_process->status = TERMINATED;
            } else if (WIFSTOPPED(status)) {
                curr_process->status = SUSPENDED;
            } else if (WIFCONTINUED(status)) {
                curr_process->status = RUNNING;
            }
        }
        curr_process = curr_process->next;
    }
}

void freeProcessList(process *process_list) {
    process *curr_process;
    while (process_list != NULL) {
        curr_process = process_list;
        process_list = process_list->next;
        free(curr_process);
    }
}

void printProcessList(process **process_list) {
    updateProcessList(process_list);
    process *curr_process = *process_list;
    process *prev = NULL;
    while (curr_process != NULL) {
        printf("PID\t\tCommand\t\tSTATUS\n");
        char *status;
        switch (curr_process->status) {
            case RUNNING:
                status = "Running";
                break;
            case SUSPENDED:
                status = "Suspended";
                break;
            case TERMINATED:
                status = "Terminated";
                break;
        }
        printf("%d\t%s\t%s\n", curr_process->pid, curr_process->cmd->arguments[0], status);
        
        if (curr_process->status == TERMINATED) {
            if (prev == NULL) {
                *process_list = curr_process->next;
            } else {
                prev->next = curr_process->next;
            }
            process *to_delete = curr_process;
            curr_process = curr_process->next;
            free(to_delete);
        } else {
            prev = curr_process;
            curr_process = curr_process->next;
        }
    }
}

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
    } else {
        // Parent process
        cmdLine *cmd = (cmdLine *)malloc(sizeof(cmdLine));
        cmd->arguments = command;
        cmd->argCount = 0;
        while (command[cmd->argCount] != NULL) {
            cmd->argCount++;
        }
        cmd->blocking = 1;

        addProcess(&process_list, cmd, pid);
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

void manipulateProcess(char *command, pid_t pid, int signal, process **process_list) {
    if (kill(pid, signal) == -1) {
        perror("kill failed");
    } else {
        if (signal == SIGTSTP) {
            updateProcessStatus(*process_list, pid, SUSPENDED);
        } else if (signal == SIGCONT) {
            updateProcessStatus(*process_list, pid, RUNNING);
        } else if (signal == SIGINT) {
            updateProcessStatus(*process_list, pid, TERMINATED);
        }
        printf("%s process %d succeeded\n", command, pid);
    }
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

        // Handle internal commands
        if (strncmp(input, "procs", 5) == 0) {
            printProcessList(&process_list);
            continue;
        } else if (strncmp(input, "wake", 4) == 0) {
            pid_t pid = atoi(input + 5);
            manipulateProcess("Wake", pid, SIGCONT, &process_list);
            continue;
        } else if (strncmp(input, "suspend", 7) == 0) {
            pid_t pid = atoi(input + 8);
            manipulateProcess("Suspend", pid, SIGTSTP, &process_list);
            continue;
        } else if (strncmp(input, "kill", 4) == 0) {
            pid_t pid = atoi(input + 5);
            manipulateProcess("Kill", pid, SIGINT, &process_list);
            continue;
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

    freeProcessList(process_list);
    return 0;
}