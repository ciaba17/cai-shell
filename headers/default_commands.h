#ifndef DEFAULT_COMMANDS_H
#define DEFAULT_COMMANDS_H

#define DEF_CMDS_NUM 3

typedef int (*command_func)(int argc, char* argv[]);

typedef struct {
    char* name;
    command_func func;
} Default_command;

int cmd_cd(int argc, char* argv[]);
int cmd_ls(int argc, char* argv[]);
int cmd_exit(int argc, char* argv[]);

extern Default_command default_commands[DEF_CMDS_NUM];

#endif