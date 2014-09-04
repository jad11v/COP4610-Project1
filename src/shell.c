#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char previous_working_directory[256];

void handleCommand(char*[], int);
void execCommand(char*, char* []);
void shell();

int main(int argc, char* argv[])
{
	shell();
	return 0;
}

void shell()
{
	getcwd(previous_working_directory, 256);
    while (1)
    {
    	  int user_argc = 0;
    	  char* user_argv[128];
      	char response[256];
      	char hostname[256];
      	char working_directory[256];
      	char* tok_response;

      	gethostname(hostname, 256);
      	getcwd(working_directory, 256);
      
      	printf("%s@%s:%s $ ", getlogin(), hostname, working_directory);
      	fgets(response, 256, stdin);
      	response[strlen(response) - 1] = '\0'; // remove '\n' character
      	tok_response = strtok(response, " ");
      
      	while (tok_response != NULL)
      	{
	    	user_argv[user_argc++] = tok_response;
	       	tok_response = strtok(NULL, " ");
	  	}
      
      	handleCommand(user_argv, user_argc);
    }
}

void handleCommand(char* args[], int argc)
{
    if (argc == 0)
    {
      	// no command, so just return
    	return;
    } 
    else if (strcmp(args[0], "exit") == 0)
    {
    	if (args[1] != NULL)
	    {
	        int exit_value = atoi(args[1]);
	        exit(exit_value);
	    }
        else
	    {
	    	exit(0);
	    }
    } 
    else if (strcmp(args[0], "cd") == 0)
    {
        int return_value = 0;
        char previous_working_directory_copy[256];
        strncpy(previous_working_directory_copy, previous_working_directory, 256); // copy over previous working directory before overwriting it
        getcwd(previous_working_directory, 256); // keep track (in global variable) of working directory before change
        if (args[1] == NULL || strcmp(args[1], "") == 0 || 
        	strcmp(args[1], "~") == 0 || strcmp(args[1], "~/") == 0)
	    {
	    	return_value = chdir(getenv("HOME")); // go to home directory
	    }
        else if (strcmp(args[1], "-") == 0)
  	    {
  	    	// go to previous working directory and print its path
  	       	return_value = chdir(previous_working_directory_copy);
  	       	printf("%s\n", previous_working_directory_copy);
  	    }
        else
  	    {
  	    	return_value = chdir(args[1]); // go to specified directory
  	    }
        
        if (return_value == -1)
  	    {
  	    	printf("Error: No such file or directory.\n");
  	    }
    }
    else if (strcmp(args[0], "ioacct") == 0)
    {
        int pid = getpid();
        char proc_file_name[100];
        char* truncated_command[100];
        int truncated_argc = 0;
        char io_info[50];
        int io_value;
      
        sprintf(proc_file_name, "/proc/%d/io", pid); // create filename /proc/pid/io
        FILE* proc_file = fopen(proc_file_name, "r"); // open the proc file for reading
      
        // execute command as normal
        // remove "ioacct" from command, create new command array
        int i;
      	for (i = 1; i < argc; ++i)
      	{
      		truncated_command[i-1] = args[i];
      		++truncated_argc;
      	}

      	// now reprocess the command
      	handleCommand(truncated_command, truncated_argc);
      
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
    else // other commands
    {
    	if (args[0][0] == '/') // absolute path given
    	{
    		execCommand(args[0], args);
    	}
    	else // absolute path not given
    	{
			char* path = getenv("PATH"); // PATH environment variable
			char* program = strdup(args[0]); // make duplicate of program name
			char test_path[256]; // to be used to locate program
			char* tok_path; 

			// chop up the PATH and test each one for the existence of the specified program
			tok_path = strtok(path, ":");
    	
        while (tok_path != NULL)
    		{
	   			//printf("%s\n", "i am stuck in a loop");
          sprintf(test_path, "%s/%s", tok_path, program);
          if (strstr(test_path, ".") != NULL) // if the path contains a '.', such as in /.bin, we want to skip it
          { 
            tok_path = strtok(NULL, ":");
            continue;
          }
	   			if (access(test_path, F_OK) == 0)
	   			{
	   				break; // found the program
	   			}
	   		  tok_path = strtok(NULL, ":");
			   }
			   
         
         if(strcmp(program, "ls") == 0 && argc == 1 )
         {
          char curr_dir[256];
          getcwd(curr_dir, 256);
          args[1] = curr_dir;
          args[2] = NULL;

         }
         printf("%s is located here: %s\n", program, test_path);

			       // so now use that path that was found
           	execCommand(test_path, args);
        }
    }
}

void execCommand(char* cmd, char* args[])
{
	int child_status;
   	pid_t pid = fork();

    if (pid < 0)
	{
		// process creation failed
    }
    else if (pid == 0)
    {
		execv(cmd, args);

    	printf("%s: Command not found.\n", args[0]);
		exit(0);
    }
    else 
    {
		wait(&child_status);
    }
}