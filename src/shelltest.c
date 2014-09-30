#include <fcntl.h> // for open()
#include <libgen.h> // for chdir()
#include <stdio.h> // for standard i/o
#include <stdlib.h> // for exit() and getenv()
#include <string.h> // for manipulating c-strings
#include <sys/stat.h> // for file permissions
#include <sys/types.h> // for pid_t data type
#include <sys/wait.h> // for wait()
#include <unistd.h> // for UNIX functions
#define PATH getenv("PATH")
char previous_working_directory[256]; // global variable for keeping track of previous working directory

int handleCommand(char*[], int);
int execCommand(char*, char* []);
void handlePosition(char*[],char*, int);
void shell();
void exitCommand(char*[], int);
int cdCommand(char*[],int);
void ioacctCommand(char*[],int);
int otherCommand(char*[], int);
int isArgInArray(char*[], char*, int argc);
int posOfArgInArray(char*[], char*, int argc);

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

int handleCommand(char* args[], int argc)
{
    int pid = 0;
    if (argc == 0)
    {
      // no command, so just return
    	return 0;
    }
    else if (isArgInArray(args, "&", argc)) // background processes
    {
        if (posOfArgInArray(args, "&", argc) == argc-1) // check for proper syntax
        {
          /*int background_status;
          pid_t background_pid;*/
          args[argc-1] = NULL; // remove & from command
          --argc; // decrement arg count

          /*background_pid = fork();

          if (background_pid < 0)
          {
            // process creation failed
          }
          else if (background_pid == 0)
          {
            handleCommand(args, argc);
            exit(0);
          }
          else
          {
            waitpid(-1, &background_status, WNOHANG); // don't wait for child to finish
          }
          */
          handleCommand(args, argc);
        }
        else
        {
          printf("Error: Incorrectly placed ampersand.\n");
        }
    }
    else if (isArgInArray(args, ">", argc)) // output redirection
    {
      handlePosition(args,">",argc);
    }
    else if (isArgInArray(args, ">>", argc)) // output redirection with append
    {
      handlePosition(args,">>",argc);
    }
    else if (isArgInArray(args, "<", argc)) // input redirection
    {
      handlePosition(args,"<", argc);
    }
   	else if (strcmp(args[0], "exit") == 0)
    {
    	exitCommand(args, argc);
    }
   	else if (strcmp(args[0], "cd") == 0)
    {
    	pid = cdCommand(args, argc);
    }
   	else if (strcmp(args[0], "ioacct") == 0)
    {
    	ioacctCommand(args, argc);
    }
    else // other commands
    {
    	pid = otherCommand(args, argc);
    }
    return pid;
}

int execCommand(char* cmd, char* args[]) // for foreground processes
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
  	  waitpid(-1, &child_status, 0); // wait for child to finish
    }

    return pid;
}

void exitCommand(char* args[], int argc)
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

int cdCommand(char * args[], int argc)
{
    int return_value = 0;
    char previous_working_directory_copy[256];
    strncpy(previous_working_directory_copy, previous_working_directory, 256); // copy over previous working directory before overwriting it
    getcwd(previous_working_directory, 256); // set previous_working_directory to the current one
 	  if (argc == 1 || strcmp(args[1], "~") == 0 || strcmp(args[1], "~/") == 0)
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
      strncpy(previous_working_directory, previous_working_directory_copy, 256); // restore the old previous_working_directory since chdir failed
    }

    return getpid();
}

void ioacctCommand(char* args[], int argc)
{
    int pid;
    char proc_file_name[100];
    char* truncated_command[100];
    char io_info[25];
    int io_value;
    FILE* proc_file;

    // remove "ioacct" from command, create new command array
    int i;
    for (i = 1; i < argc; ++i)
    {
   		truncated_command[i-1] = args[i];
    }

    --argc; // decrement argument count

    // now reprocess the command
    pid = handleCommand(truncated_command, argc);

    sprintf(proc_file_name, "/proc/%d/io", pid); // create filename /proc/pid/io
    /*puts(proc_file_name);
    if (access(proc_file_name, F_OK) == 0)
    {
    	printf("File okay\n");
    }*/
    proc_file = fopen(proc_file_name, "r"); // open the proc file for reading

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

int otherCommand(char* args[], int argc )
{
    int pid;
  	if (args[0][0] == '/') // absolute path given
  	{
    	pid = execCommand(args[0], args);
    }
    else // absolute path not given
    {
    	char* program = args[0]; // make duplicate of program name
    	char path_var_copy[1000];
    	char test_path[256]; // to be used to locate program
    	char* tok_path;

    	strncpy(path_var_copy, PATH, 1000); // copy PATH into new string

    	// chop up the PATH and test each one for the existence of the specified program
    	tok_path = strtok(path_var_copy, ":");

      	while (tok_path != NULL)
      	{
        	sprintf(test_path, "%s/%s", tok_path, program);
       		if (strstr(test_path, ".") != NULL) // if the path contains a '.', such cdas in /.bin, we want to skip it
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

       	if (strcmp(program, "ls") == 0 && argc == 1)
      	{
        	char curr_dir[256];
        	getcwd(curr_dir, 256);
        	args[1] = curr_dir;
       		args[2] = NULL;
       	}
       	//printf("%s is located here: %s\n", program, test_path);

       	// now use the path that was found
    	 pid = execCommand(test_path, args);
    }

    return pid;
}

int isArgInArray(char* args[], char* arg, int argc)
{
  int i;
  for (i = 0; i < argc; ++i)
  {
    if (strcmp(args[i], arg) == 0)
    {
      return 1;
    }
  }
  return 0;
}

int posOfArgInArray(char* args[], char* arg, int argc)
{
  int i;
  for (i = 0; i < argc; ++i)
  {
    if (strcmp(args[i], arg) == 0)
    {
      return i;
    }
  }
  return -1;
}

void handlePosition(char*args[],char* direction, int argc)
{
    
    int posOfOperator = posOfArgInArray(args, ">", argc);
	  if (posOfOperator == argc-1 || argc < 3)
	  {  
      if(strcmp(direction, ">") == 0 || strcmp(direction,">>") == 0)
      {
        printf("Error: Malformed output redirection.\n");
      }
      else
      {
        printf("Error: Malformed input redirection.\n");
      }
	    
	  }
	  else
	  {
	    char* leftArgs[50];
	    char* out_filename = args[argc-1];
	    int i;
	    int file_descriptor;
	    int stdout_dup = dup(STDOUT_FILENO);
	    int leftArgCount = posOfOperator;

	    for (i = 0; i < leftArgCount; ++i)
	    {
	      leftArgs[i] = args[i];
	    }

	    // open file in write-only mode, clear it if it already exists, and create it if it doesn't; also set read/write rights for user
	    file_descriptor = open(args[argc-1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	    if (file_descriptor < 0) // file opening somehow failed
	    {
	      printf("Error: Failed to open file.\n");
	    }
	    else
	    {
	      close(STDOUT_FILENO); // close original stdout
	      dup2(file_descriptor, STDOUT_FILENO); // redirect stdout to file
	      handleCommand(leftArgs, leftArgCount); // process left-hand command
	      close(file_descriptor); // close the file
	      dup2(stdout_dup, STDOUT_FILENO); // restore stdout
	      close(stdout_dup);
	    }
	  }
	}

