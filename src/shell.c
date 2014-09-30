/*
Developers: Joseph Deleeuw and Robert Massicotte

Description: A basic shell program that repeatedly prompts the user for input and executes the inputted commands.
*/

#include <fcntl.h> // for open()
#include <stdio.h> // for standard i/o
#include <stdlib.h> // for exit() and getenv()
#include <string.h> // for manipulating c-strings
#include <sys/stat.h> // for file permissions
#include <sys/types.h> // for pid_t data type
#include <sys/wait.h> // for wait()
#include <unistd.h> // for UNIX functions

char previous_working_directory[256]; // global variable for keeping track of previous working directory

void shell();
void handleCommand(char*[], int);
void execCommand(char*, char*[]);
void exitCommand(char*[], int);
void cdCommand(char*[],int);
void wordCommand(char*[],int);
void ioacctCommand(char*[],int);
void otherCommand(char*[], int);
void redirectOutput(char*[], int);
void redirectAppend(char*[], int);
void redirectInput(char*[], int);
void pipedCommand(char*[], int);
void runInBackground(char*[], int);
int isArgInArray(char*[], char*, int);
int posOfArgInArray(char*[], char*, int);

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
    	return;  // no command, so just return
    } 
    else if (isArgInArray(args, "&", argc)) // background processes
    {
      runInBackground(args, argc);
    }
    else if (isArgInArray(args, ">", argc)) // output redirection
    {
      redirectOutput(args, argc);
    }
    else if (isArgInArray(args, ">>", argc)) // output redirection with append
    {
      redirectAppend(args, argc);
    }
    else if (isArgInArray(args, "<", argc)) // input redirection
    {
      redirectInput(args, argc);
    }
    else if (isArgInArray(args, "|", argc)) // piped command
    {
      pipedCommand(args, argc);
    }
   	else if (strcmp(args[0], "exit") == 0) // exit shell command
    {
    	exitCommand(args, argc);
    }
   	else if (strcmp(args[0], "cd") == 0) // change directory
    {
    	cdCommand(args, argc);
    }
   	else if (strcmp(args[0], "ioacct") == 0) // view I/O information
    {
    	ioacctCommand(args, argc);
    }
    else if (strcmp(ars[0], "wordcheck") ==0)
    {

    }
    else // other commands, such as ls, grep, cat, etc.
    {
    	otherCommand(args, argc);
    }
}

void execCommand(char* cmd, char* args[])
{
  	int child_status;
    FILE* proc_file;
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

void cdCommand(char* args[], int argc)
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
    
    if (return_value < 0)
    {
   		printf("Error: No such file or directory.\n");
      strncpy(previous_working_directory, previous_working_directory_copy, 256); // restore the old previous_working_directory since chdir failed
    }
}

void ioacctCommand(char* args[], int argc)
{
  int i;
  int io_value;
  char io_info[20];
  char* truncated_command[256];
  char proc_file_name[256];
  FILE* proc_file;

  // remove "ioacct" from command, create new command array
  for (i = 1; i < argc; ++i)
  {
    truncated_command[i-1] = args[i];
  }

  --argc; // decrement argument count

  handleCommand(truncated_command, argc); // now reprocess the command

  sprintf(proc_file_name, "/proc/%d/io", getpid());
  proc_file = fopen(proc_file_name, "r");

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

void otherCommand(char* args[], int argc )
{
  	if (args[0][0] == '/') // absolute path given
  	{
      execCommand(args[0], args);
    }
    else // absolute path not given
    {
    	char* program = args[0]; // make duplicate of program name
      int path_length = strlen(getenv("PATH")) + 1;
    	char path_var[path_length];
    	char test_path[256]; // to be used to locate program
    	char* tok_path; 

    	strncpy(path_var, getenv("PATH"), path_length); // get the PATH
      path_var[path_length-1] = '\0';

    	// chop up the PATH and test each one for the existence of the specified program
    	tok_path = strtok(path_var, ":");
    
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

    	 execCommand(test_path, args); // now use the path that was found
    }
}

void redirectOutput(char* args[], int argc)
{
    int posOfOperator = posOfArgInArray(args, ">", argc);
    if (posOfOperator == argc-1 || argc < 3)
    {
      printf("Error: Malformed output redirection.\n");
    }
    else
    {
      char* leftArgs[256];
      char* out_filename = args[argc-1];
      int i;
      int output_file;
      int stdout_dup = dup(STDOUT_FILENO);
      int leftArgCount = posOfOperator;

      for (i = 0; i < leftArgCount; ++i)
      {
        leftArgs[i] = args[i];
      }

      // open file in write-only mode, clear it if it already exists, and create it if it doesn't; also set read/write rights for user
      output_file = open(out_filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
      if (output_file < 0) // file opening somehow failed
      {
        printf("Error: Failed to open file.\n");
      }
      else
      {
        dup2(output_file, STDOUT_FILENO); // duplicates output_file file descriptor using STDOUT_FILENO
        handleCommand(leftArgs, leftArgCount); // process left-hand command
        close(output_file); // close the file
        dup2(stdout_dup, STDOUT_FILENO); // restore stdout
      }
    }
}

void redirectAppend(char* args[], int argc)
{
    int posOfOperator = posOfArgInArray(args, ">>", argc);
    if (posOfOperator == argc-1 || argc < 3)
    {
      printf("Error: Malformed output redirection.\n");
    }
    else
    {
      char* leftArgs[256];
      char* out_filename = args[argc-1];
      int i;
      int output_file;
      int stdout_dup = dup(STDOUT_FILENO);
      int leftArgCount = posOfOperator;

      for (i = 0; i < leftArgCount; ++i)
      {
        leftArgs[i] = args[i];
      }

      // open file in write-only mode, append to it if it already exists, and create it if it doesn't; also set read/write rights for user
      output_file = open(out_filename, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
      if (output_file < 0) // file opening somehow failed
      {
        printf("Error: Failed to open file.\n");
      }
      else
      {
        dup2(output_file, STDOUT_FILENO); // duplicates output_file file descriptor using STDOUT_FILENO
        handleCommand(leftArgs, leftArgCount); // process left-hand command
        close(output_file); // close the file
        dup2(stdout_dup, STDOUT_FILENO); // restore stdout
      }
    }
}

void redirectInput(char* args[], int argc)
{
    int posOfOperator = posOfArgInArray(args, "<", argc);
    if (posOfOperator == argc-1 || argc < 3)
    {
      printf("Error: Malformed input redirection.\n");
    }
    else
    {
      char* leftArgs[256];
      char* in_filename = args[argc-1];
      int i;
      int input_file;
      int stdin_dup = dup(STDIN_FILENO);
      int leftArgCount = posOfOperator;

      for (i = 0; i < leftArgCount; ++i)
      {
        leftArgs[i] = args[i];
      }

      // open file in read-only mode
      input_file = open(in_filename, O_RDONLY);
      if (input_file < 0) // file most likely does not exist
      {
        printf("Error: No such file or directory.\n");
      }
      else
      {
        dup2(input_file, STDIN_FILENO); // duplicates input_file file descriptor using STDIN_FILENO
        handleCommand(leftArgs, leftArgCount); // process left-hand command
        close(input_file); // close the file
        dup2(stdin_dup, STDIN_FILENO); // restore stdin
      }
    }
}

void pipedCommand(char* args[], int argc)
{
    int i;
    int fin = -1;
    int fout = -1;
    int prev_pipe_pos = -1;
    int child_status;

    for (i = 0; i < argc; ++i) // loop through each argument
    {
      pid_t pid;
      int pfd[2];
      if (strcmp(args[i], "|") == 0) // if the argumnent is a pipe, we want to process the commands to the left of it
      {
        int left_i = 0; // used to populate leftArgs
        char* leftArgs[256]; // the left-side hand arguments
        int leftArgCount;
        int j;

        if (prev_pipe_pos != -1)
        {
          leftArgCount = i - prev_pipe_pos - 1; 
          j = prev_pipe_pos + 1;
        }
        else
        {
          leftArgCount = i;  // if prev_pipe_pos == -1, then we're at the beginning and the number of left args is just i
          j = 0; // if prev_pipe_pos == -1, then we're at the beginning, so we start populating leftArgs from the 0th index
        }

        prev_pipe_pos = i; // want to update prev_pipe_pos for future use

        // populate leftArgs
        for (j; j < i; ++j)
        {
          leftArgs[left_i++] = args[j];
        }

        pipe(pfd); // create the pipe
        fout = pfd[1]; // set fout to write end of pipe

        // create a child process and let it do the command execution
        pid = fork();
        if(pid == 0)
        {
            // verify descriptors
            if(fin != -1 && fin != 0) 
            {
                dup2(fin, 0);
                close(fin);
            }

            if(fout != -1 && fin != 1) 
            {
                dup2(fout, 1);
                close(fout);
            }

            // process the command and terminate
            handleCommand(leftArgs, leftArgCount);
            exit(-1);
        }

        wait(&child_status); // wait on child
        close(fout);
        close(fin);
        fin = pfd[0]; // set fin to read end of pipe
      }
      else if (i+1 == argc) // we're on the last argument
      {
        int left_i = 0; // used to populate leftArgs
        char* leftArgs[256]; // the left-side hand arguments
        int leftArgCount = i - prev_pipe_pos; // since we're on the last argument, the number of left args is i - prev_pipe_pos
        int j = prev_pipe_pos + 1; // since we're on the last argument, we just start populating leftArgs from the argument after the previous pipe

        // populate leftArgs
        for (j; j < i+1; ++j)
        {
          leftArgs[left_i++] = args[j];
        }

        // no need to pipe() here
        fout = -1; // set fout to -1, since we're just reading the final input now

        // create a child process and let it do the command execution
        pid = fork();
        if (pid == 0)
        {
            // verify descriptors
            if( fin != -1 && fin != 0) 
            {
                dup2(fin, STDIN_FILENO);
                close(fin);
            }

            if (fout != -1 && fin != 1) 
            {
                dup2(fout, STDOUT_FILENO);
                close(fout);
            }

            // process the command and terminate
            handleCommand(leftArgs, leftArgCount);
            exit(-1);
        }

        wait(&child_status); // wait on child
        close(fout);
        close(fin);
      }
    }

    wait(&child_status); // wait for any children
}

void runInBackground(char* args[], int argc)
{
    if (posOfArgInArray(args, "&", argc) == argc-1) // check for proper syntax
    {
      args[argc-1] = NULL; // remove & from command
      --argc; // decrement arg count
      handleCommand(args, argc);
    }
    else
    {
      printf("Error: Incorrectly placed ampersand.\n");
    }
}

// returns 1 if the argument is in the array; 0 if not
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

// returns the position of the argument in the array; -1 if not in the array
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
void wordCommand(char* args[], int argc)
{
  if (argc != 1)
  {

  int result;
  char *argument = args[1];
  File *f = fopen("/usr/share/dict/words","r");
  while(fscanf(f,"%s",temp) != EOF)
  {
    if(strcmp(temp, args[1]) == 0) 
    {
      result = 1;
    }
  }
  if(result == 1 )
    {
      printf("The word is in the dicionary.");
    }
  else
  {
    printf("the word is not in the dictionary");
  }
  
}