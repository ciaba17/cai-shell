#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#include "default_commands.h"

// --- EXIT ---
bool is_numeric(const char *s) {
    if (*s == '\0') return 0; // If string is empty then it's not numeric

    const char *p = s;
    if (*p == '-') p++; // Permette il segno meno iniziale
    if (*p == '\0') return false; 

    while (*p) {
        if (!isdigit((unsigned char)*p))
            return false;
        p++;
    }
    return true;
}

int cmd_exit(int argc, char* argv[]) {
    if (argc == 1) {
        exit(0);
    }
    else if (argc == 2) {
        if (!is_numeric(argv[1])){
            fprintf(stderr, "bash: numeric argument needed\n");
            return 1;
        }
        int nExit = atoi(argv[1]);
        exit(nExit);
    }
    else {
        fprintf(stderr, "bash: too many arguments\n");
        return 1;
    }
}

// --- CD ---
char* get_home_dir() {
	struct passwd* pw = getpwuid(getuid());
	char* dir = pw->pw_dir;
	return dir;
}

int cmd_cd(int argc, char* argv[]) {
    if (argc > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }

    char* path;

    if (argc == 1) {
        path = get_home_dir();
    } else {
        // Handle tilde expansion
        if (argv[1][0] == '~') {
            char* home = get_home_dir();
            path = malloc(strlen(home) + strlen(argv[1]));
            sprintf(path, "%s%s", home, argv[1] + 1);
            if (chdir(path) != 0) {
                perror("cd");
                free(path);
                return 1;
            }
            free(path);
            return 0;
        }
        path = argv[1];
    }

    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

// --- LS --- 
// (not necessary, since ls is usually built in)
int print_list(char* target, char* flags) {
    DIR* dir = opendir(target);
    if (!dir) {
        perror("opendir");
        return 1;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Filter for -a
        if (entry->d_name[0] == '.' && (!flags || !strchr(flags, 'a')))
            continue;

        if (flags && strchr(flags, 'l')) {
            // Search for the system file stats
            struct stat fileStat;
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", target, entry->d_name);
            if (stat(path, &fileStat) < 0) {
                perror("stat");
                continue;
            }
        
            // Permissions
            printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
            printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
            printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
            printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
            printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
            printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
            printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
            printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
            printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
            printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
        
            // Print link count, user, group, and size
            printf(" %ld", fileStat.st_nlink);
            struct passwd *pw = getpwuid(fileStat.st_uid);
            struct group  *gr = getgrgid(fileStat.st_gid);
            printf(" %s %s", pw->pw_name, gr->gr_name);
            printf(" %ld", fileStat.st_size);
        
            // Print last modification timestamp
            char timebuf[80];
            strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&fileStat.st_mtime));
            printf(" %s", timebuf);
        }
        // Print the file name
        printf(" %s\n", entry->d_name);
    }
    closedir(dir);
    return 0;
}

int cmd_ls(int argc, char* argv[]) {
    char* target = ".";
    char* flags = NULL;
    if (argc == 1) {
        // Default: list current directory
    } else if (argc == 2) {
        if (argv[1][0] == '-') {
            flags = argv[1] + 1; // Remove the '-'
        } else {
            target = argv[1];
        }
    } else if (argc == 3) {
        if (argv[1][0] == '-') {
            flags = argv[1] + 1;
            target = argv[2];
        } else {
            fprintf(stderr, "ls: invalid arguments\n");
            return 1;
        }
    } else {
        fprintf(stderr, "ls: too many arguments\n");
        return 1;
    }
    return print_list(target, flags);
}

// Built-in commands
Default_command default_commands[DEF_CMDS_NUM] = {
    {"exit", cmd_exit},
    {"cd", cmd_cd},
    {"cails", cmd_ls}
};

