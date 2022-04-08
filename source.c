#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

int background_allow = 1;

struct cmds {
	char** arg;
	char* input_file;
	char* output_file;
	int background;
};

// Similar to function in Exploration: Signal Handling API
void handle_SIGTSTP(int signo) {
	// Unsure of how to pass a variable in here, so used a global variable.
	if (background_allow == 1) {
		background_allow = 0;
		char* message = "Entering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 50);
		fflush(stdout);
	}
	else {
		background_allow = 1;
		char* message = "Exiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 30);
		fflush(stdout);
	}
}

/*
*	Change current working directory using the path given.
*/

void changedir_path(struct cmds* cmd_line) {
	int confirm;
	
	confirm = chdir(cmd_line->arg[1]);
	if (confirm == -1) {
		printf("Error: No such directory found.\n");
		return;
	}
}

/*
*	Change current working directory to the home directory.
*/

void changedir_home(struct cmds* cmd_line) {
	char path[1000];

	strcpy(path, getenv("HOME"));
	chdir(path);
}

/*
*	Check the command input to see if the $$ is present.
*	If it is, replace it with the current PID.
*/

void check_expansion(char* cmd) {
	int pid_num = getpid();
	char spid_num[2];

	sprintf(spid_num, "%d", pid_num);
	
	for (int j = 0; j < strlen(cmd) + 1; j++) {
		if (cmd[j] == '$' && cmd[j + 1] == '$') {
			char* temp = calloc(1, sizeof(char)); //Save up to the first $
			//char* second_half = cmd + j + 2;		  //Create a pointer to everything after the second $
			strncpy(temp, cmd, j);					  //Copy the first half to the temp
			strcat(temp, spid_num);					  //Append the PID # onto the end of the first half
		//	strcat(temp, second_half);				  //Append the second half onto the temp
			//cmd = (char*)realloc(cmd, strlen(temp));
			strcpy(cmd, temp);						  //Copy the temp string to become the new command string
			break;
		}
	}
}

/*
*	Print the exit status of the last process.
*/

void print_status(int exit_status) {
	if (WIFEXITED(exit_status)) {
		printf("Exited with status: %d\n", WEXITSTATUS(exit_status));
		fflush(stdout);
	}
	else {
		printf("Exited via signal: %d\n", WTERMSIG(exit_status));
	}
}

void execute_env_cmd(struct cmds* cmd_line, int* exit_status, struct sigaction sa) {
	// Similar to execl example from Exploration: Process API - Executing a New Program
	int input_fd, output_fd, result;
	pid_t new_child = fork();

	switch (new_child) {
	// The fork failed
	case -1:
		perror("fork()\n");
		exit(1);
		break;
	case 0:
		//Child process will ignore SIGTSTP
		sa.sa_handler = SIG_IGN;
		sigaction(SIGTSTP, &sa, NULL);

		//Check to see if there is an input redirect file
		if (cmd_line->input_file != NULL) {
			// Similar to Output redirection example from Exploration: Processes and I/O in Module 5
			input_fd = open(cmd_line->input_file, O_RDONLY);

			//Opening the file failed
			if (input_fd == -1) {
				perror("source open()");
				exit(1);
			}
			result = dup2(input_fd, 0);
			//Making the redirected FD failed
			if (result == -1) {
				perror("dup2");
				exit(1);
			}
			//
			fcntl(input_fd, F_SETFD, FD_CLOEXEC);
		}
		//Check to see if there is an output redirect file
		if (cmd_line->output_file != NULL) {
			// Similar to Output redirection example from Exploration: Processes and I/O in Module 5
			output_fd = open(cmd_line->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);

			//Opening the file failed
			if (output_fd == -1) {
				perror("source open()");
				exit(1);
			}

			result = dup2(output_fd, 1);
			//Making the redirected FD failed
			if (result == -1) {
				perror("dup2");
				exit(1);
			}
			fcntl(output_fd, F_SETFD, FD_CLOEXEC);
		}

		if (execvp(cmd_line->arg[0], cmd_line->arg)) {
			//If the program ends up in here, there was an error on execvp
			perror("execvp");
			exit(1);
		}
		break;
	default: 
		//Check if the process is meant to be background
		if (background_allow == 1 && cmd_line->background == 1) {
			pid_t new_pid = waitpid(new_child, exit_status, WNOHANG); //A new process is created but don't wait for it.
			printf("Background PID: %d\n", new_child);
			fflush(stdout);
		}
		else {
			new_child = waitpid(new_child, exit_status, 0); //Process is foreground, wait for it to end
		}

		//On the next command, check if any background process have ended.
		while ((new_child = waitpid(-1, exit_status, WNOHANG)) > 0) { //waitpid returns anything positive, that means a process has ended. The -1 is used to watch for any PID.
			printf("Child %d terminated\n", new_child);
			print_status(*exit_status);
			fflush(stdout);
		}
	}
}

/*
*	Process the command and check if it will be one of the built in commands
*/

void cmd_parse(struct cmds* cmd_line, const int num_args, int* exit_status, struct sigaction sa) {
	if (strncmp(cmd_line->arg[0], "cd", strlen("cd")) == 0) {
		if (num_args > 1) {
			changedir_path(cmd_line);
		}
		else {
			changedir_home(cmd_line);
		}
	}
	else if (strncmp(cmd_line->arg[0], "status", strlen("status")) == 0) {
		print_status(*exit_status);
	}
	else {
		execute_env_cmd(cmd_line, exit_status, sa);
	}
}

int main(int argc, char** argv) {
	size_t cmd_size = 2048;
	char* cmd_in = calloc(cmd_size + 1, sizeof(char));
	char* token;
	char* saveptr;
	struct cmds* cmd_line;
	int cont;
	int num_args;
	int exit_status = 0;
	
	// Signal Handlers

	// Ignore SIGINT
	struct sigaction SIGINT_action = { 0 };
	SIGINT_action.sa_handler = SIG_IGN;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	// Customize SIGTSTP
	struct sigaction SIGTSTP_action = { 0 };
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	do {
		
		cmd_line = malloc(sizeof(struct cmds));
		cont = 1;

		// Get command input
		printf(": ");
		fflush(stdout);
		fgets(cmd_in, cmd_size, stdin); // fgets was used instead of getline due to some bugs when using SIGTSTP where it would break the stdin
		fflush(stdin);
		check_expansion(cmd_in);	// Check for $$ expansion

		// Remove \n from input
		for (int i = 0; i < strlen(cmd_in); i++) {
			if (cmd_in[i] == '\n') {
				cmd_in[i] = '\0';
			}
		}

		// Process the command into the struct
		// If the first character of the input is a comment or nothing, skip everything and rerun the loop
		if (cmd_in[0] == '\0' || cmd_in[0] == '#' || strcmp(cmd_in, "") == 0) {
		}
		else {
			num_args = 0;
			// Take in the first token, which is the command, and check if it's exit. Otherwise, process the rest of the string
			token = strtok_r(cmd_in, " \0", &saveptr);
			cmd_line->arg = calloc(1, sizeof(char));
			cmd_line->arg[0] = calloc(strlen(token) + 1, sizeof(char));
			strcpy(cmd_line->arg[0], token);
			if (strncmp(cmd_line->arg[0], "exit", strlen("exit")) == 0) {
				cont = 0;
			}
			else {
				cmd_line->input_file = NULL;
				cmd_line->output_file = NULL;
				cmd_line->background = 0;

				// A special case needed to be made for echo. It expects a full string as its arguement. This makes the rest of the string
				// (after the command "echo") a single argument to allow the command to function properly.
				if (strcmp(cmd_line->arg[0], "echo") == 0) {
					num_args++;
					token = strtok_r(NULL, "\0\n", &saveptr);
					if (token != NULL) {
						cmd_line->arg[1] = calloc(strlen(token) + 1, sizeof(char));
						strcpy(cmd_line->arg[1], token);
						num_args++;
					}
				}
				else {
					token = strtok_r(NULL, " \0", &saveptr);
					num_args++;
					// Check for special cases and fill in the struct as necessary
					while(token != NULL) {
						// Check for background
						if (strcmp(token, "&") == 0) {
							cmd_line->background = 1;
						}
						// Check for input redirect. Up to the next space is the file to be redirected.
						else if (strcmp(token, "<") == 0) {
							token = strtok_r(NULL, " \0", &saveptr);
							cmd_line->input_file = calloc(strlen(token) + 1, sizeof(char));
							strcpy(cmd_line->input_file, token);
						}
						// Check for output redirect. Up to the next space is the file to be redirected.
						else if (strcmp(token, ">") == 0) {
							token = strtok_r(NULL, " \0", &saveptr);
							cmd_line->output_file = calloc(strlen(token) + 1, sizeof(char));
							strcpy(cmd_line->output_file, token);
						}
						// If none of the above apply, then it is a flag for the command
						else {
							cmd_line->arg[num_args] = calloc(strlen(token) + 1, sizeof(char));
							strcpy(cmd_line->arg[num_args], token);
							num_args++;
						}
						// Move to next word / arguement to process
						token = strtok_r(NULL, " \0", &saveptr);
					}
				}
				cmd_line->arg[num_args] = NULL;
				cmd_parse(cmd_line, num_args, &exit_status, SIGTSTP_action);
			}
			/*for (int i = 0; i < num_args; i++) {
				free(cmd_line->arg[i]);
			}*/
		}
		free(cmd_line);
		
	} while (cont != 0);
	return 0;
}