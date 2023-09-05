#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXLINE 1000
#define DEBUG 0

int childpid1 = -1;
int childpid2 = -1;

void signal_handler(int signal)
{
	if (childpid1 > 0) {
		if(DEBUG) printf("(DEBUG) Killing childpid1 %d\n", childpid1);
		if(kill(childpid1, signal) != 0) {
			perror("killing childpid1");
			printf("Error killing childpid1 %d! Exiting...\n", childpid1);
			exit(1);
		}
		childpid1 = -1;
	}

	if (childpid2 > 0) {
		if(DEBUG) printf("(DEBUG) Killing childpid2 %d\n", childpid2);
		if(kill(childpid2, signal) != 0) {
			perror("killing childpid2");
			printf("Error killing childpid2 %d! Exiting...\n", childpid2);
			exit(1);
		}
		childpid2 = -1;
	}
	write(STDOUT_FILENO, "\n% ", 3);
}

int main(int argc, char *argv[])
{

	// register signal handler
	signal(SIGINT, signal_handler);

	while (1)
	{

		// argv
		char *command[100];

		printf("%% ");

		// user string
		char buffer[100];
		if (fgets(buffer, MAXLINE, stdin) == NULL && feof(stdin))
		{
			printf("Couldn't read from standard input. End of file? Exiting ...\n");
			exit(1);
		}

		int words = parse_command(buffer, command);

		int i;
		if (DEBUG)
			printf("(DEBUG) Parsed command (%d): ", words);
		for (i = 0; command[i] != NULL; i++)
		{
			if (DEBUG)
				printf("'%s', ", command[i]);
		}
		printf("\n");

		if (!strcmp(command[0], "exit"))
		{
			exit(0);
		}

		int background = 0;
		if (!strcmp(command[words - 1], "&"))
		{
			background = 1;
			command[words - 1] = NULL;
			words--;
		}

		int output_redirection = 0;
		char *redirection_file = "";
		if (words >= 3 && !strcmp(command[words - 2], ">"))
		{
			output_redirection = 1;
			redirection_file = command[words - 1];
			command[words - 2] = NULL;
			words--;
		}

		char *command1[100];
		char *command2[100];
		int piping = parse_pipe(command, command1, command2);
		int pipe_fd[2];
		if (piping)
		{
			if (pipe(pipe_fd) != 0)
			{
				perror("opening pipe");
			}
			if (DEBUG)
				printf("(DEBUG) Opened pipe. Read fd %d and write fd %d\n", pipe_fd[0], pipe_fd[1]);

			childpid2 = fork(); // for piping, this is writing process
			if (childpid2 == 0)
			{ // child
				if (DEBUG)
					printf("(DEBUG) This is process 2 (the write process). Closing stdout and copying in the pipe fd %d.\n", pipe_fd[1]);
				if (DEBUG)
					printf("(DEBUG) First three argv are: '%s' '%s' '%s'\n", command1[0], command1[1], command1[2]);
				if (close(STDOUT_FILENO) != 0)
				{
					perror("closing stdout");
					exit(1);
				}
				if (close(pipe_fd[0]) != 0)
				{
					perror("closing pipe read");
					exit(1);
				}
				if (dup2(pipe_fd[1], STDOUT_FILENO) != STDOUT_FILENO)
				{
					perror("piping write stream");
					exit(1);
				}
				if(execvp(command1[0], command1) != 0) {
					perror("executing command2");
					printf("Process 2: error launching command %s. Wrong name?\n", command1[0]);
					exit(1);
				}
			}
		}

		childpid1 = fork(); // for piping, this is the reading process
		if (childpid1 == 0) // child
		{

			if (output_redirection)
			{
				if (DEBUG)
					printf("(DEBUG) Redirecting output to file %s.\n", redirection_file);

				if (close(STDOUT_FILENO) != 0)
				{
					perror("closing stdout");
					exit(1);
				}

				int fd = open(redirection_file, O_WRONLY | O_CREAT);
				if (fd < 0)
				{
					perror("opening file");
					exit(1);
				}
				chmod(redirection_file, S_IRUSR | S_IWUSR);

				if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO)
				{
					perror("duplicating file descriptor");
					exit(1);
				}
			}

			if (piping)
			{
				if (DEBUG)
					printf("(DEBUG) This is process 1 (the read process). Closing stdin and copying in the pipe fd %d.\n", pipe_fd[0]);
				if (DEBUG)
					printf("(DEBUG) First three argv are: '%s' '%s' '%s'\n", command2[0], command2[1], command2[2]);
				if (close(STDIN_FILENO) != 0)
				{
					perror("closing stdin");
					exit(1);
				}
				if (close(pipe_fd[1]) != 0)
				{
					perror("closing pipe write");
					exit(1);
				}
				if (dup2(pipe_fd[0], STDIN_FILENO) != STDIN_FILENO)
				{
					perror("piping read stream");
					exit(1);
				}
				if(execvp(command2[0], command2) != 0) {
					perror("executing command2");
					printf("Process 1: error launching command %s. Wrong name?\n", command2[0]);
					exit(1);
				}
			}

				if(execvp(command[0], command) != 0) {
					perror("executing command");
					printf("Process 1: error launching command %s. Wrong name?\n", command[0]);
					exit(1);
				}
		}
		else // parent
		{
			if (piping)
			{
				if (close(pipe_fd[0]) != 0)
				{
					perror("closing pipe read in main process");
					exit(1);
				}
				if (close(pipe_fd[1]) != 0)
				{
					perror("closing pipe write in main process");
					exit(1);
				}
			}
			if (!background)
			{
				if(childpid1 > 0) waitpid(childpid1);
				childpid1 = -1;
				if(childpid2 > 0) waitpid(childpid2);
				childpid2 = -1;
			}
			else
			{
				if (DEBUG)
					printf("(DEBUG) Running command in background...\n");
			}
		}
	}
}

int parse_command(char buffer[], char *command[100])
{
	int i;
	command[0] = &buffer[0];
	int j = 1;
	for (i = 0; buffer[i] != '\0'; i++)
	{
		if (buffer[i] == ' ')
		{
			buffer[i] = '\0';
			command[j] = &buffer[i + 1];
			j++;
		}
		else if (buffer[i] == '\n')
		{
			buffer[i] = '\0';
		}
	}
	command[j] = NULL;
	return j;
}

int parse_pipe(char *command[], char *command1[], char *command2[])
{
	int i;
	int j = 1;
	int passed_pipe = 0;
	command1[0] = command[0];
	for (i = 1; command[i] != NULL; i++)
	{ // start at 1
		if (!strcmp(command[i], "|"))
		{
			command1[j] = NULL;
			passed_pipe = 1;
			j = 0;
		}
		else
		{
			if (passed_pipe)
			{
				command2[j] = command[i];
			}
			else
			{
				command1[j] = command[i];
			}
			j++;
		}
	}
	command2[j] = NULL;
	return passed_pipe;
}
