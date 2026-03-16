#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>

#include "interpreter.h"
#include "default_commands.h"

int interpret_and_execute_command(int argc, char* argv[]) {
    if (argc == 0)
        return 0;

    // Built-in commands
    for (int i = 0; i < DEF_CMDS_NUM; i++) {
        if (strcmp(argv[0], default_commands[i].name) == 0) {
            return default_commands[i].func(argc, argv);
        }
    }

    bool background = false;
    if (argc > 0 && strcmp(argv[argc-1], "&") == 0) {
        background = true;
        argv[argc-1] = NULL; // Remove & from the arguments
        argc--;
    }

    // External commands
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    } 
    else if (pid == 0) { // Child
        execvp(argv[0], argv);
        perror("exec failed");
        exit(1);
    } 
    else { // Parent
        if (!background) {
            int status;
            waitpid(pid, &status, 0); // Wait for child
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            return 1;
        } 
        else {
            printf("[pid %d] started in background\n", pid);
            return 0;
        }
    }
}