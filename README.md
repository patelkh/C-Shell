# C-Shell
## **Shell basics**

A shell is a program that executes other programs on behalf of the user. At the core of its functionalities, we have the fork and execve functions. With the first one, the shell creates child processes. It’s in their context that new programs are executed. execve takes care of the actual execution of programs. These child processes, and the group of processes that they create during their execution, are called jobs.

In general, a shell can execute a job in the foreground or in the background. At any time, there’s at most one job being executed in the foreground. The shell halts until the foreground process is terminated or interrupted. On the other hand, many background processes can run concurrently. After loading a background process, the shell continues its read and evaluation loop. Background jobs are executed by appending a & character at the end of the command line instruction (for example: : my_program &).

Once a job terminates its execution, it remains in the system memory until the shell, its parent process, “reaps” it. A process that is terminated but hasn’t been reaped is a zombie child. One important task of the shell is to reap all of its children. Not doing so creates memory leaks.

## **Basic shell functionalities**

This basic shell has two main functionalities:

Program execution: The shell executes programs that are in the current working directory (: my_program) or in the path specified by the user (: /bin/ls) in the context of child processes. Any strings, separated by spaces, that come after the program name are passed to it as arguments when it’s executed: : my_program arg1 arg2 ....

Job control: The shell keeps track of all running jobs, which can be executed either in the foreground or in the background. The shell commands fg and bg can be used to move a job between the foreground and the background. Additionally, the foreground job can be terminated or stopped when the user types Ctrl+C and Ctrl+Z, respectively.

When the shell quits, it forces the termination of all of it’s running or stopped children and reaps them.

## **How to use my C Shell**
### The Command Prompt: 
The shell uses the colon (:) symbol as a prompt for each command line. The general syntax of a command line is: 
> command [arg1 arg2 ...] [< inputfile] [> outputfile] [&] 
where items in square brackets are optional. You can assume that a command is made up of words separated by spaces. 
The special symbols <, > and & are recognized but they must be surrounded by spaces like other words. 
If the command is to be executed in the background, the last word must be &. 
If the & character appears anywhere else, it is treated as normal text.
If standard input or output is to be redirected, the > or < words followed by a filename word must appear after all the arguments. Input redirection can appear before or after output redirection.
This shell does not support any quoting; so arguments with spaces inside them are not possible. 
This shell supports command lines with a maximum length of 2048 characters, and a maximum of 512 arguments.
Error checking on the syntax of the command line is not available. 

### Comments and Blanks: 
Blank lines and comments are allowed. Any line that begins with the # character is a comment line and is ignored. Mid-line comments, such as the C-style //, is not supported. A blank line (one without any commands) reprompts user for a command.

### Expansion of Variable $$: 
The shell expands any instance of $$ in a command into the process ID of the shell.

### Built-in Commands: 
exit, cd, and status. All other built-in commands are simply passed on to a member of the exec() family of functions. If the user tries to run one of these built-in commands in the background with the & option, the shell ignores that option and runs the command in the foreground anyway.

exit
The exit command exits the shell. It takes no arguments. When this command is run, it kills any other processes or jobs that the shell has started before it terminates itself.

cd
The cd command changes the working directory of the shell. By itself - with no arguments - it changes to the directory specified in the HOME environment variable This is typically not the location where the shell was executed from, unless the shell executable is located in the HOME directory, in which case these are the same. This command can also take one argument: the path of a directory to change to. The cd command supports both absolute and relative paths.

status
The status command prints out either the exit status or the terminating signal of the last foreground process ran by your shell. If this command is run before any foreground command is run, then it simply returns the exit status 0.

### Executing Other Commands: 
the shell executes any commands other than the 3 built-in command by using fork(), exec() and waitpid(). Whenever a non-built in command is received, the parent (i.e., smallsh) forks off a child. The child uses a function from the exec() family of functions to run the command. It uses the PATH variable to look for non-built in commands, and it allows shell scripts to be executed. If a command fails because the shell could not find the command to run, then the shell prints an error message and set the exit status to 1. A child process terminates after running a command (whether the command is successful or it fails).

### Input & Output Redirection: 
Use dup2() for input or output redirection. The shell performs redirection before using exec() to run the command. An input file redirected via stdin is opened for reading only; if the shell cannot open the file for reading, it prints an error message and set the exit status to 1 (but don't exit the shell). Similarly, an output file redirected via stdout is opened for writing only; it is truncated if it already exists or created if it does not exist. If the cannot open the output file it prints an error message and set the exit status to 1 (but don't exit the shell). Both stdin and stdout for a command can be redirected at the same time.

### Signals SIGINT & SIGTSTP
SIGINT: A CTRL-C command from the keyboard sends a SIGINT signal to the parent process and all children at the same time (this is a built-in part of Linux).
The shell ignores SIGINT
Any children running as background processes also ignores SIGINT
A child running as a foreground process terminates itself when it receives SIGINT
The parent does not attempt to terminate the foreground child process; instead the foreground child (if any) terminates itself on receipt of this signal.
If a child foreground process is killed by a signal, the parent immediately print out the number of the signal that killed it's foreground child process (see the example) before prompting the user for the next command.

SIGTSTP: A CTRL-Z command from the keyboard sends a SIGTSTP signal to your parent shell process and all children at the same time (this is a built-in part of Linux).
A child, if any, running as a foreground process ignores SIGTSTP.
Any children running as background process also ignores SIGTSTP.
When the parent process running the shell receives SIGTSTP, the shell displays an informative message immediately if it's sitting at the prompt, or immediately after any currently running foreground process has terminated. The shell then enters a state where subsequent commands can no longer be run in the background. In this state, the & operator is simply ignored, i.e., all such commands are run as if they were foreground processes. If the user sends SIGTSTP again, then the shell displays another informative message immediately after any currently running foreground process terminates. The shell then returns back to the normal condition where the & operator is once again honored for subsequent commands, allowing them to be executed in the background.

