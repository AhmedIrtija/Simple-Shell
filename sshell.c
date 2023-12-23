#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLINE_MAX 512
#define ARG_MAX 16
#define TOKEN_MAX 32

typedef enum ParsingState
{
	Done_parsing,		 // when finished parsing sucessfully
	Next_cmd,			 // ready to receive next cmd from user
	Failed_Parsing = -1, // when parsing fails
} ParsingState;

/************************** LINKED LIST *************************/
typedef struct Cmd_Node
{
	char *command;		// command
	char *arg[ARG_MAX]; // argument
	char *file;			// file name if redirection occurs
	int argu_size;		// track how many arguments there are
	int redirectErr;
	struct Cmd_Node *next;
} Cmd_Node;

Cmd_Node *head;
Cmd_Node *list;

// building the linked list
void add_list(char *cmd, char *argv[], char *name, int size, int redirectErr)
{
	Cmd_Node *new_Node = (Cmd_Node *)malloc(sizeof(Cmd_Node));
	// allocate space for node
	new_Node->command = cmd;
	char *args[ARG_MAX + 2];
	args[0] = cmd;
	for (int i = 0; i < size; i++)
	{
		args[i + 1] = argv[i];
	}
	args[size + 1] = NULL;

	for (int i = 0; i < size + 1; i++)
	{
		new_Node->arg[i] = args[i];
	}
	new_Node->file = name;
	new_Node->argu_size = size;
	new_Node->redirectErr = redirectErr;

	if (!list)
	{
		list = head;
	}

	list->next = new_Node;
	list = list->next;
}

void delete_cmd(void)
{
	Cmd_Node *cur_Node = head;
	while (cur_Node != NULL)
	{
		free(cur_Node);
		cur_Node = cur_Node->next;
	}
}

/********************* REDIRECTION *******************************/
ParsingState change_dir(char *filename)
{

	if (chdir(filename) == -1)
	{ // error changing directory
		fprintf(stderr, "Error: could not change directory\n");
		return (EXIT_FAILURE);
	}
	return Next_cmd;
}

/********************* REDIRECTION *******************************/
// while parsing, check for redirection indicator,
// if user entered '<', we will separate the command and save file name
char *parse_redirection(char *cmdline, char **filename, char **redirectErr)
{ // check for redirection and the filename
	char *command;
	char copyLine[CMDLINE_MAX];
	int fileCheck = 0;

	strcpy(copyLine, cmdline);

	// Check for extra features (stderr redirection)
	for (int i = 0; i < (int)strlen(copyLine); i++)
	{

		if (copyLine[i] == '&')
		{
			*redirectErr = "found";
		}
		if (copyLine[i] == '>')
		{
			fileCheck = 1;
		}
	}
	command = strtok(copyLine, ">");
	*filename = strtok(NULL, " >&"); // get filename
	if (fileCheck == 1 && *filename == NULL)
	{
		fprintf(stderr, "Error: file missing\n");
		return NULL;
	}

	return command;
}

/************************** PARSING ***************************/
// Parse through each command and check for redirection then put them in list
ParsingState parse_cmdline(char *cmdline, int redirectErr)
{
	char copyCMD[CMDLINE_MAX];
	char *command;
	char *arg_List[ARG_MAX];
	char *actualArg[ARG_MAX];
	char *file = NULL;
	char *redirectERR = NULL;
	char *argu;
	int size = -1;
	int arg_count = 0;

	strcpy(copyCMD, cmdline);
	command = parse_redirection(copyCMD, &file, &redirectERR);
	if (command == NULL)
	{
		return Failed_Parsing;
	}

	argu = strtok(command, " ");

	command = argu;

	for (int i = 0; argu != NULL; ++i) //&& i < ARG_MAX
	{
		argu = strtok(NULL, " "); // gets all commands
		arg_List[i] = argu;
		size++;
		++arg_count;
	}

	for (int j = 0; j < size; j++)
	{
		actualArg[j] = arg_List[j];
	}

	/************* check BuiltIn Commands [cd and pwd]************/
	if (!strcmp(command, "cd"))
	{
		char *dir = actualArg[0];
		if (dir)
		{
			change_dir(dir);
		}
		else
		{
			fprintf(stderr, "Error: cannot cd into directory\n");
			return (EXIT_FAILURE);
		}
		return Next_cmd;
	}
	/*if user inputs pwd*/
	if (!strcmp(command, "pwd"))
	{
		char buf[CMDLINE_MAX];
		char *path = getcwd(buf, CMDLINE_MAX);
		fprintf(stdout, "%s\n", path);

		return Next_cmd;
	}

	if (arg_count > ARG_MAX)
	{
		fprintf(stderr, "Error: Too many arguments\n");
		return (EXIT_FAILURE);
	}
	if (sizeof(argu) > TOKEN_MAX)
	{
		fprintf(stderr, "Error: invalid token length\n");

		return (EXIT_FAILURE);
	}

	int redirection = 0;
	if (!redirectERR || redirectErr == 1)
	{
		redirection = 1;
	}
	add_list(command, actualArg, file, size, redirection);

	return Done_parsing;
}

ParsingState multiCMD(char *line, int *amountCmd)
{ // Check for pipes to get all the commands
	char *commands[CMDLINE_MAX];
	int commandSize = 0;
	char lines[CMDLINE_MAX];
	int redirectErr = 0;
	strcpy(lines, line);

	// Check for extra features (stderr redirection)
	for (int i = 0; i < (int)strlen(lines); i++)
	{

		if (lines[i] == '&')
		{
			redirectErr = 1;
		}
	}
	int returnVal;

	char *singleLine;
	singleLine = strtok(lines, "|&");

	while (singleLine != NULL)
	{
		commands[commandSize++] = singleLine;
		singleLine = strtok(NULL, "|&");
	}

	for (int i = 0; i < commandSize; i++)
	{
		returnVal = parse_cmdline(commands[i], redirectErr);
	}
	*amountCmd = commandSize;
	return returnVal;
}

void print_completed(int stats, char cmd[])
{
	fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(stats));
}

void forking(char *allCmd, int size)
{
	Cmd_Node *traverse = head->next;
	int stats;

	if (size == 1)
	{
		pid_t pid;
		pid = fork();

		if (pid > 0) // parent
		{
			int stats;
			waitpid(pid, &stats, 0); // wait for child process
			print_completed(stats, allCmd);
		}
		else if (pid == 0) // child
		{
			if (traverse->file != NULL)
			{
				int fd;
				fd = open(traverse->file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
			}

			execvp(traverse->command, traverse->arg);
			fprintf(stderr, "Error: unable to execute command\n");
			exit(1);
			return;
		}
		return;
	}
	else if (size == 2)
	{
		int fd[2];
		pipe(fd);

		pid_t pid1, pid2;
		pid1 = fork();

		if (pid1 == 0)
		{
			// Child 1 process
			close(fd[0]); // Close unused read pipe
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]); // Close the pipe write
			execvp(traverse->command, traverse->arg);
			// Execute first command
			exit(1);
		}
		else
		{
			pid2 = fork();

			if (pid2 == 0)
			{
				// Child 2 process
				close(fd[1]); // Close unused write pipe
				dup2(fd[0], STDIN_FILENO);
				if (traverse->redirectErr == 1)
				{
					dup2(fd[0], STDERR_FILENO);
				}
				close(fd[0]); // Close the pipe read

				traverse = traverse->next;
				if (traverse->file != NULL)
				{
					int fd;
					fd = open(traverse->file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
					dup2(fd, STDOUT_FILENO);
					close(fd);
				}

				execvp(traverse->command, traverse->arg);
				// Execute second command
				fprintf(stderr, "Error: unable to execute command\n");
				exit(1);
			}
			else
			{
				// Parent process
				close(fd[0]); // Close unused read pipe
				close(fd[1]); // Close unused write pipe
				waitpid(pid1, &stats, 0);
				waitpid(pid2, &stats, 0);
				print_completed(stats, allCmd);
			}
		}
	}
	else if (size == 3)
	{
		int fd1[2];
		int fd2[2];
		pipe(fd1);
		pipe(fd2);

		pid_t pid1, pid2, pid3;
		pid1 = fork();

		if (pid1 == 0)
		{
			// Child 1 process
			close(fd1[0]); // Close unused read pipe
			dup2(fd1[1], STDOUT_FILENO);
			close(fd1[1]); // Close the pipe write
			execvp(traverse->command, traverse->arg);
			// Execute first command
			exit(1);
		}
		else
		{
			pid2 = fork();

			if (pid2 == 0)
			{
				// Child 2 process
				close(fd1[1]); // Close unused write pipe
				dup2(fd1[0], STDIN_FILENO);
				if (traverse->redirectErr == 1)
				{
					dup2(fd1[0], STDERR_FILENO);
				}
				close(fd1[0]); // Close the pipe read
				close(fd2[0]); // Close unused read pipe
				dup2(fd2[1], STDOUT_FILENO);
				close(fd2[1]); // Close the pipe write

				traverse = traverse->next;
				if (traverse->file != NULL)
				{
					int fd;
					fd = open(traverse->file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
					dup2(fd, STDOUT_FILENO);
					close(fd);
				}
				execvp(traverse->command, traverse->arg);
				// Execute second command
				fprintf(stderr, "Error: unable to execute command\n");
				exit(1);
			}
			else
			{
				pid3 = fork();

				if (pid3 == 0)
				{
					// Child 3 process
					close(fd2[1]); // Close unused write pipe
					dup2(fd2[0], STDIN_FILENO);
					if (traverse->redirectErr == 1)
					{
						dup2(fd2[0], STDERR_FILENO);
					}
					close(fd2[0]); // Close the pipe read
					traverse = traverse->next;
					if (traverse->file != NULL)
					{
						int fd;
						fd = open(traverse->file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
						dup2(fd, STDOUT_FILENO);
						close(fd);
					}
					execvp(traverse->command, traverse->arg);
					// Execute third command
					fprintf(stderr, "Error: unable to execute command\n");
					exit(1);
				}
				else
				{
					// Parent process
					close(fd1[0]);			  // Close unused read pipe
					close(fd1[1]);			  // Close unused write pipe
					close(fd2[0]);			  // Close unused read pipe
					close(fd2[1]);			  // Close unused write pipe
					waitpid(pid1, &stats, 0); // Wait for first child
					waitpid(pid2, &stats, 0); // Wait for second child
					waitpid(pid3, &stats, 0); // Wait for third child
					print_completed(stats, allCmd);
				}
			}
		}
	}
	else
	{
		// Invalid number of commands
		fprintf(stderr, "Error: invalid number of commands\n");
	}
}

int main(void)
{

	while (1)
	{
		char *nl;
		int stats = 0;
		int cmdSize = 0;
		head = (Cmd_Node *)malloc(sizeof(Cmd_Node));
		list = head;
		// command line buffer
		char cmd[CMDLINE_MAX] = "";
		/* Print prompt */
		printf("sshell@ucd$ ");
		fflush(stdout);

		/* Get command line */
		fgets(cmd, CMDLINE_MAX, stdin);

		/* Print command line if stdin is not provided by terminal */
		if (!isatty(STDIN_FILENO))
		{
			printf("%s", cmd);
			fflush(stdout);
		}

		/* Remove trailing newline from command line */
		nl = strchr(cmd, '\n');

		if (nl)
			*nl = '\0';

		/*if user inputs exit*/
		if (!strcmp(cmd, "exit"))
		{
			fprintf(stderr, "Bye...\n");
			print_completed(stats, cmd);
			break;
		}

		ParsingState parsed = multiCMD(cmd, &cmdSize);
		if (parsed == Failed_Parsing)
		{
			continue;
		}

		forking(cmd, cmdSize);
	}

	delete_cmd();

	return EXIT_SUCCESS;
}