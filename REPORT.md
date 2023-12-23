# Project 1: Simple Shell
---

## Purpose

The purpose of this project was to imitate a shell using function and concepts
learned in class. This includes file manipulation, handling command line
arguments, creating a Makefile and making use of system calls that Unix-like
systems would usually handle. Through this project, we learned how processes are
launched in a shell and how they are configured. At the time of submission, our
program can only handle up to two commands for a given pipeline command entered
in by the user.

A shell is a computer program that provides an interface for users to access
the operating system's services. By entering instructions into a command-line
interface, users can interact with the system's file system, launch programs,
and complete other tasks. These commands are translated by the shell before
being sent to the operating system for execution.

## Summary

Our simple shell, called sshell, interprets command line instructions from the
user. When the sshell is ready for the user to input a command, our sshell
prints the prompt "sshell@ucd$ ". Once the user inputs a command, our simple
shell attempts to execute the command. If there are errors in the given
command, our simple shell will print an error message. If there are not errors,
our sshell will print the result along with the exit status and will be ready
to accept a new command from the user. Our sshell also allows for pipeline
commands, commands that involve redirection, and changing directories, just like
a regular shell. Our shell program provides extra features like redirection of
the standard error of commands to files or pipes, and handle simple environment
variables.

### Implementation

Our program uses the "fork, execute, and wait" method to implement the shell.
The main function has a while loop which will print "sshell@ucd$ " to indicate
that the program is ready to execute a command. The program gets the command
from the user using fgets(). If the user entered "exit", it terminate the
program, and returns 0. Then, the commandline gets parsed using the function
mulitCMD(). After parsing is sucessful, the main function will call forking()
where it will begin the forking process. This function will then create a 
child process which execute the command using execvp(). We choose to
use execvp() instead of any of the other types of exec() function because the
"p" specicification means that it will tell the function to use the PATH
environment variable to find and using the file name at the filename argument.
The program attempts to recreate a terminal using basic commands with 
complicated arguments. 

### Parsing Command Line

If exit was not the command entered by the user, we parse the commandline array
in a function called multiCMD which returns the return value. This function
takes in the command line and checks if there are any pipelines in the command
line and returns the return the state of parsing that is resulted from calling
the function parse_cmdline().If a pipeline was found, we parse the line
differently. This parse_cmdline() function, tokenizes the commandline and
it checks if the command line entered by user had a valid commandline argument
and number of tokens. This function calls another function which checks for a
redirection symbol and removes the symbol so that it is executed properly. The
parse_cmdlin() function returns EXIT_FAILURE for command line errors, Next_cmd
or Done_parsing when parsing is sucessful. After parsing the command line, we
put them into linkedlist where the data can be save into a node for each 
command. In the linked list, the data saved in each node is the command(s), 
argument(s) and the filename.

### Builtin Commands (exit, cd, and pwd)

Before parsing the command, we check if the user entered "exit" as a command.
If the user did enter "exit" it immediately exits the while loop and returns 0.
That way, it can skip having to fork() and make a child process. During the
parsing process, we check for commands "pwd" and "cd". If the user enters the
"pwd" command, we use the function getcwd() to get the current path and print
the current path. When "cd" is entered, the directory is changed to the
specified directory with the use of the function change_dir(). If the directory
is not valid, it returns an error status. For this project, we assume the user 
enters in the correct number of arguments for cd and pwd.

### Output Redirection

Our program handles file redirection in a function called check_redirection().
This function is called in the initial parsing function, parse_cmdline(),
which returns the command line without the redirection symbol. It accepts the
command line array of strings and a filename pointer. In the check_redirection()
function, it will look for the symbol '>' and will remove it from the
command line array and saves the filename. As explained before, this function
returns the command back to parse_cmdline(). Once returned, the function 
parse_cmdline() will process the filename into the linked list. Later, once the 
forking() function is called, the filename will be used as a recognition for 
redirection of the output. If the filename is NULL then the output is going to 
standard output while if it's not then it either looks for the file or creates 
a new one to redirect the output. 

### Pipeline

Shell pipeline commands are used to pass the output of one command as the input 
for another command. The output of the first command is passed as input to the 
second command by the shell. This is a very powerful tool and can be used to 
execute a couple of commands in one line.

Our program contains all the instances of fork() inside a function called
forking(). After the command is successfully parsed, the main function will call
the forking() function. This function takes in the command line as an array of
strings and the size of the command line. The first thing the function does is
initialize a variable for creating a LinkedList. If there was
only one command entered in, the function will only call fork() once and will
execute the child process before the parent process. If the linked list has a
file associated with it, the program will call dup2() to interact with the file
and create one if the file is not already created. When the single command is
executed in the child process, the parent will print out the completed message
and error status to the user and will exit the function forking().

When there is more than one command, this indicates that the user wants to
execute a pipeline command. If there are two commands, we fork() once creating a
child process and creating a pipe. Inside child process 1, it closes the unused
read pipe and calls dup2 to point the STDIN_FILENO to the read pipe. Then, it
closes the read pipe and executes this first command.

In child process 2, it calls fork() again. The child will close the unused write
pipe, calls dup2() to point the STDIN_FILENO to the read pipe, and closes the
read pipe. If there was a file redirection, it calls dup2() again and redirects
the output to the file. If there user does not want to redirect the output to a
file, then it executes the process. In the parent process of this child process
2, it closes the read and writes pipes and calls the function waitpid() on child
process 1 and child process 2. We use waitpid() instead of wait() because we
want to make sure we wait for the correct child process among all the other
children created when we make more than one fork() call.
