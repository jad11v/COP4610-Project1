#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *trimnewline(char *str);
void shell();

int main(int argc, char* argv[])
{
	shell();
	return 0;
}

void shell()
{
	char response[256];
	char hostname[256];
	char working_directory[256];
	gethostname(hostname, 256);
	do
	{
		getcwd(working_directory, 256);
		printf("%s@%s:%s $ ", getlogin(), hostname, working_directory);
		fgets(response, 256, stdin);
		trimnewline(response);
		if (strcmp(response, "exit") == 0)
		{
			exit(EXIT_SUCCESS);
		}
	} while (1);
}

char *trimnewline(char *str)
{
	int i;
	for(i = 0; i < strlen(str); i++)
	{

		if (str[i] == '\n')
		{

			str[i] = '\0';
		}
	}
	return str;
}