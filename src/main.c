#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <pwd.h>
#include <dirent.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "interpreter.h"

#define ARGS_MAX 64

void ignore_sigint(int) {}
char* get_shell_prompt();
int tokenize(char* line, char* argv[]);

int main() {
	signal(SIGINT, ignore_sigint); // Ignore SIGINT (Ctrl+C) to keep shell running
	system("stty -echoctl"); // Disable echoing of control characters (like ^C)
	printf("\033[2J\033[H"); // ANSI sequence to clear the screen

	char* shellPrompt;
	char* argv[ARGS_MAX];
	int argc;
	char* line;
	int status;

	// --- MAIN SHELL LOOP ---
	while (true) {
		// Clean up any background processes that have finished (avoiding zombies)
		while (waitpid(-1, &status, WNOHANG) > 0); 

		shellPrompt = get_shell_prompt();
		
		// --- INPUT ---
		line = readline(shellPrompt); // Wait for user input

		if (line == NULL) break; // Exit on Ctrl+D (EOF)
			
		if (*line) {
			add_history(line); // Save command to history
			
			char* cmd_start = line;
			char* next_ptr;
			bool in_quotes = false;
			
			while (cmd_start && *cmd_start) {
				next_ptr = NULL;
				bool is_and_and = false;
				bool is_bg = false;

				// Look for command separators: &&, ;, or & (ignoring quoted strings)
				for (int i = 0; cmd_start[i] != '\0'; i++) {
					if (cmd_start[i] == '"') in_quotes = !in_quotes;
					if (!in_quotes) {
						if (cmd_start[i] == '&' && cmd_start[i+1] == '&') {
							cmd_start[i] = '\0';
							cmd_start[i+1] = '\0';
							next_ptr = &cmd_start[i+2];
							is_and_and = true;
							break;
						} else if (cmd_start[i] == ';') {
							cmd_start[i] = '\0';
							next_ptr = &cmd_start[i+1];
							break;
						} else if (cmd_start[i] == '&') {
							cmd_start[i] = '\0';
							next_ptr = &cmd_start[i+1];
							is_bg = true;
							break;
						}
					}
				}

				argc = tokenize(cmd_start, argv);
				
				// Handle background operator if it was used as a separator
				if (is_bg) {
					argv[argc++] = "&";
					argv[argc] = NULL;
				}

				if (argc > 0) {
					int exit_status = interpret_and_execute_command(argc, argv);
					
					// If && was used, stop sequence if the command failed
					if (is_and_and && exit_status != 0) break;
				}
				
				cmd_start = next_ptr;
				// Skip leading whitespace or duplicate separators
				while (cmd_start && (*cmd_start == ' ' || *cmd_start == ';')) cmd_start++; 
			}
		}

		free(shellPrompt);
		free(line);
	}
}

char* get_shell_prompt() {
	char* user = getlogin();
	if (!user) // Fallback
        user = getenv("USER");

	char hostname[256];
	gethostname(hostname, sizeof(hostname));

	char* cwd = getcwd(NULL, 0);
    if (!cwd) {
        perror("getcwd");
        cwd = strdup("unknown"); // Fallback
    }

	// Calculates the exact length
    size_t len = strlen(user) + strlen(hostname) + strlen(cwd) + 99; // extra for symbols and spaces
    char* prompt = malloc(len);
    if (!prompt) {
        perror("malloc");
        free(cwd);
        return NULL;
    }

	// Assemble the prompt
	strcpy(prompt, "╭─");
	strcat(prompt, user);
	strcat(prompt, " on ");
	strcat(prompt, hostname);
	strcat(prompt, " in ");
	strcat(prompt, cwd);
	strcat(prompt, "\n╰─❯");

	free(cwd);

	return prompt;
}

int tokenize(char* line, char* argv[]) {
    int argc = 0;
    bool in_quotes = false;
    char* start = line;

    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '"') {
            in_quotes = !in_quotes; // True for the first " and false for the second "

            // Remove the " moving the string by +1 in memory
            memmove(&line[i], &line[i+1], strlen(&line[i]));
            i--;
            continue;
        }

        if (line[i] == ' ' && !in_quotes) {
            line[i] = '\0';

            if (*start != '\0') {
                argv[argc++] = start;
            }

            start = &line[i+1]; // Get the next char
        }
    }

    if (*start != '\0') {
        argv[argc++] = start;
    }

    argv[argc] = NULL;
    return argc;
}