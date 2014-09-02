#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct
{
	int argc;
	char* argv[128];
} Args;

char previous_working_directory[256];

void handleCommand(Args*);
void shell();
Args* getUserInput();

int main(int argc, char* argv[])
{
	shell();
	return 0;
}

void shell()
{
	Args* input;
	getcwd(previous_working_directory, 256);
	while (1)
	{
		input = getUserInput();

		handleCommand(input);

		// clear input
		while (input->argc != 0)
		{
			input->argv[input->argc--] = NULL;
		}

		free(input);
	}
}

Args* getUserInput()
{
	Args* args = malloc(sizeof(Args));
	char response[256];
	char hostname[256];
	char working_directory[256];
	char* tok_response;
	args->argc = 0;
	gethostname(hostname, 256);
	getcwd(working_directory, 256);

	printf("%s@%s:%s $ ", getlogin(), hostname, working_directory);
	fgets(response, 256, stdin);
	response[strlen(response) - 1] = '\0'; // remove '\n' character
	tok_response = strtok(response, " ");
	while (tok_response != NULL)
	{
		args->argv[args->argc++] = tok_response;
		tok_response = strtok(NULL, " ");
	}
	return args;
}

void handleCommand(Args* response)
{
	if (strcmp(response->argv[0], "exit") == 0)
	{
		if (response->argv[1] != NULL)
		{
			int exit_value = atoi(response->argv[1]);
			exit(exit_value);
		}
		else
		{
			exit(0);
		}
	}
	else if (strcmp(response->argv[0], "cd") == 0)
	{
		int return_value = 0;
		char previous_working_directory_copy[256];
		strncpy(previous_working_directory_copy, previous_working_directory, 256); // copy over previous working directory before overwriting it
		getcwd(previous_working_directory, 256); // keep track (in global variable) of working directory before change
		if (response->argv[1] == NULL || strcmp(response->argv[1], "~/") == 0 )
		{
			return_value = chdir(getenv("HOME")); // go to home directory
		}
		else if (strcmp(response->argv[1], ".") == 0)
		{
			// do nothing, stay in current directory
		}
		else if (strcmp(response->argv[1], "..") == 0)
		{
			// go to parent directory
			char working_directory[256];
			getcwd(working_directory, 256);
			return_value = chdir(dirname(working_directory));
		}
		else if (strcmp(response->argv[1], "-") == 0)
		{
			// go to previous working directory and print its path
			return_value = chdir(previous_working_directory_copy);
			printf("%s\n", previous_working_directory_copy);
		}
		else
		{
			return_value = chdir(response->argv[1]); // go to specified directory
		}

		if (return_value == -1)
		{
			printf("Error: No such file or directory.\n");
		}
	}
	else if (strcmp(response->argv[0], "ioacct") == 0)
	{
		int pid = getpid();
		char proc_file_name[100];
		char io_info[50];
		int io_value;

		sprintf(proc_file_name, "/proc/%d/io", pid); // create filename /proc/pid/io
		FILE* proc_file = fopen(proc_file_name, "r"); // open the proc file for reading
		
		// execute command as normal

		// then read "read_bytes" and "write_bytes" and write values out
		while (fscanf(proc_file, "%s %d", io_info, &io_value) != EOF)
		{
			if (strcmp(io_info, "read_bytes:") == 0)
			{
				printf("bytes read: %d\n", io_value);
			}
			else if (strcmp(io_info, "write_bytes:") == 0)
			{
				printf("bytes written: %d\n", io_value);
			}
		}

		fclose(proc_file); // close the file
	}
	else // other commands like ls, rm, cat, etc.
	{
        int pid = fork();
        if (pid == 0) 
		{
        	char full_command_path[100];
			sprintf(full_command_path, "/bin/%s", response->argv[0]); // create full path, e.g. /bin/ls
        	printf("Full: %s\n", full_command_path);
            if (execv(full_command_path, response->argv)) 
            {
                printf("%s: Command not found.\n", response->argv[0]);
            }

            	exit(0);
        }
        else 
        {
       		wait(&pid);
        }
	}
}
