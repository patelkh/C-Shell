#define _POSIX_C_SOURCE 200809L
#include <stdio.h> //fgets()
#include <stdlib.h> //malloc()
#include <unistd.h> //stdin, stdout, stderr, dup2, execv, getpid, fork 
#include <string.h> //strtok()
#include <sys/types.h> //pid_t
#include <sys/wait.h> //waitpid()
#include <fcntl.h> //open()
#include <signal.h> //sigaction()
#include <stdbool.h> //bool

/*
Author: Khushboo Patel
smallsh.c - custom shell with a subset of features well-known to shells such as bash.
Provides a prompt for running bash-like commands (e.g., echo hello world)
Provides expansion for the variable $$ ($$ returns process id)
Supports input and output redirection (e.g., ls > junk)
Supports running commands in foreground and background processes
Handles SIGINT and SIGSTP signals
To run the shell, go to the terminal
enter: gcc -std=gnu99 -Wall -Wextra -Wpedantic -Werror -o smallsh smallsh.c
enter: ./smallsh
enter: command 
command syntax: command [arg1 arg2 ...] [< input_file] [> output_file] [&]
*/

volatile sig_atomic_t fg=0;

//store bg process-ids in linked list
struct bg_process {
    pid_t bgID;
    struct bg_process *next; 
} *head=NULL, *tail=NULL;

//add bg process-id at the end of list
void add_id(pid_t id) {
    struct bg_process *newNode;
    newNode = (struct bg_process *)malloc(sizeof(struct bg_process));
    //setup new node
    newNode->bgID = id;
    newNode->next = NULL;
    fflush(stdout);
    //add new node to the linked list 
    if(head == NULL) {
        head = newNode;
        tail = newNode;
    } else {
        //point current tail to new node  
        tail->next = newNode;
        //new node becomes the new tail
        tail = newNode;
    }
}

//delete bg process id from linked list
void delete_id(pid_t id) {
    struct bg_process *current = head; 
    if(current->bgID == id) {
        head = current->next;
    } else {
        struct bg_process *previous =  head;
        //traverse until bg process-id is found then remove
        while(current) {
            if(current->bgID == id) {
                previous->next = current->next;
            }
            previous = current;
            current = current->next; 
        }
    }
}

//terminate bg processes before exiting the shell
void kill_processes(void) {
    struct bg_process *temp = head; 
    //go on a killing spree 
    while(temp) {
        kill(temp->bgID, SIGKILL);
        temp = temp->next;
    }
}

//toggle fg on/off
void handle_sigstp() {
    if(fg==0) {
        char *message = "Entering foreground-only mode (& is now ignored)\n";
        fg=1;
        write(STDOUT_FILENO, message, strlen(message));
        fflush(stdout);
    } else {
        char *message = "Exiting foreground-only mode\n";
        fg=0;
        write(STDOUT_FILENO, message, strlen(message));
        fflush(stdout);
    }
    return;
}

//parse command line for arguments, redirection and background flag
int parse_command(char *commandLine, char *commandArgs[], char **inputFile, char **outputFile, char *bg, char *pidstr) {
    int i=0; //counts each word in the command line as an argument
    //variable expansion stuff 
    int j=0; 
    int k=0;
    int pidLen = strlen(pidstr);
    char expandedStr[2049];
    
    //<module 2><strings> Maintains context between successive calls to parse a string 
    char *saveptr;  
    char *token = strtok_r(commandLine, " \n", &saveptr);

    while(token != NULL) {
        memset(expandedStr, 0, 2049);
        //input redirection
        if(strcmp(token, "<")==0) {
            //get input file 
            token = strtok_r(NULL, " \n", &saveptr);
            // printf("Token: %s\n", token);
            *inputFile = token;
        } else if(strcmp(token, ">")==0) {
            //output redirection, get output file 
            token = strtok_r(NULL, " \n", &saveptr);
            // printf("Token: %s\n", token);
            *outputFile = token;
        } 
        else {
            //loop through and replace $$ with pid
            for(j=0; j<(int)strlen(token); j++) {
                char currentValue = *(token+j);
                char nextValue = *(token+j+1);
                if(currentValue=='$' && nextValue=='$') {
                    strncpy(&expandedStr[j+k], pidstr, pidLen);
                    k+=pidLen-2;
                    j++;
                } else {
                    strncpy(&expandedStr[j+k], &currentValue, 1);
                }
            }
            *(commandArgs+i) = strdup(expandedStr);
            i++;
        }
        //get next token
        token = strtok_r(NULL, " \n", &saveptr);
    }

    //check if the last argument is &
    if(strcmp(commandArgs[i-1], "&")==0) {
        //remove from array
        *(commandArgs+i-1) = NULL;
        if(fg==0) {
            *bg='Y';
        }
    } 

    // loop through array to view arguments
    // for(int j=0; j<i; j++) {
    //     printf("Argument %d: %s\n", j, commandArgs[j]);
    //     fflush(stdout);
    // }

    return i;
}

int main(void) {

    /*INSTRUCTIONS FOR RUNNING THIS PROGRAM
    gcc -std=gnu99 -Wall -Wextra -Wpedantic -Werror -o smallsh smallsh.c
    ./smallsh
    */

    size_t buffSize = 2049; 
    bool exitFlag = false; 
    int totalArgs=0;
    char *commandLine=NULL; 
    char *commandArgs[518]={NULL};
    char *inputFile=NULL;
    char *outputFile=NULL;
    int status=0;
    char bg='N';
    pid_t pid=getpid();
    pid_t childPid=0;
    char *pidstr;
    {   //code taken from professor's Ed post 
        int n = snprintf(NULL, 0, "%d", pid);
        pidstr = malloc((n + 1) * sizeof *pidstr);
        sprintf(pidstr, "%d", pid);
    }
    
    //code from module 5 signal handling API exploration 
    struct sigaction sa_sigint;
    memset(&sa_sigint, 0, sizeof(struct sigaction));
    //define attributes: sa_handler, sa_mask and sa_flags
    sa_sigint.sa_handler = SIG_IGN;
    sigfillset(&sa_sigint.sa_mask);
    sa_sigint.sa_flags = 0;
    //install signal handler for parent process to ignore SIGINT
    sigaction(SIGINT, &sa_sigint, NULL); 

    struct sigaction sa_sigstp;
    memset(&sa_sigstp, 0, sizeof(struct sigaction));
    sa_sigstp.sa_handler = &handle_sigstp; 
    sigfillset(&sa_sigstp.sa_mask);
    sa_sigstp.sa_flags = SA_RESTART;
    //install signal handler for parent process to handle SIGTSTP
    sigaction(SIGTSTP, &sa_sigstp, NULL); 

    //keep the shell running until user sends in the exit command
    while(!exitFlag) {
        //Get command from user
        printf(": ");
        fflush(stdout);
        int numChar = getline(&commandLine, &buffSize, stdin);
        if(numChar==-1) {
            commandLine=NULL;
            continue;
        }
        
        // use for debugging
        // char command[] = "sleep &";    
        // commandLine = command;

        //Check for comments and blank lines        
        if(strncmp(&commandLine[0], "#", 1)==0 || strncmp(&commandLine[0], "\n", 1)==0) {
            //reset used variables
            totalArgs=0;
            memset(commandLine, 0, 2049);
            continue;
        } 

        //Parse user command 
        totalArgs = parse_command(commandLine, commandArgs, &inputFile, &outputFile, &bg, pidstr);

        //check for built-in exit, cd and status commands
        if(strcmp(*commandArgs, "exit")==0) {
            //kill child processes before exiting
            kill_processes();
            exitFlag = true; 
            break;
        } else if (strcmp(*commandArgs, "cd")==0) {
            //change working directory of smallsh
            if(totalArgs==1) {
                chdir(getenv("HOME"));
            } else {
                if(chdir(commandArgs[1])==-1) {
                    fprintf(stderr, "Invalid directory\n");
                    fflush(stderr);
                }   
            } 
        } else if (strcmp(*commandArgs, "status")==0) {
            if(WIFEXITED(status)) {
                printf("exit value %d\n", WEXITSTATUS(status));
                fflush(stdout);
            } else {
                printf("terminated by signal %d\n", WTERMSIG(status));
                fflush(stdout);
            }
        } else {    //non-built in command
            childPid = fork();
            switch(childPid) {
                case -1:
                    fprintf(stderr, "fork() failed.\n");
                    fflush(stderr);
                    exit(1);
                    break;
                //child process
                case 0:
                    if(bg == 'N') {
                        //install ^C signal handler for foreground processes 
                        sa_sigint.sa_handler = SIG_DFL;
                        sa_sigint.sa_flags = SA_RESTART;
                        sigaction(SIGINT, &sa_sigint, NULL);
                    }

                    // handle input redirection
                    if(inputFile == NULL) {
                        //redirect to /dev/null for background command
                        if(bg == 'Y' && fg == 0) {
                            //redirect standard input to /dev/null; read from /dev/null
                            int sourceFD = open("/dev/null", O_RDONLY);
                            if(sourceFD==-1) {
                                fprintf(stderr, "Error in redirecting stadard input to /dev/null.\n");
                                fflush(stderr);
                                exit(1);
                                break;
                            }
                            int result = dup2(sourceFD, STDIN_FILENO);
                            if( result==-1) {
                                fprintf(stderr, "sourceFD dup2().\n");
                                fflush(stderr);
                                exit(1);
                                break;
                            }
                            close(sourceFD);
                        }
                        //do nothing for foreground command  
                    } else if(inputFile != NULL) {
                        //open input file 
                        int sourceFD=open(inputFile, O_RDONLY);
                        if(sourceFD==-1){
                            fprintf(stderr, "Failed to open the input file.\n");
                            fflush(stderr);
                            exit(1);
                            break;
                        }
                        //printf("Successfully opened: %s\n", inputFile);
                        fflush(stdout);
                        //redirect stdin, get input from file
                        int result=dup2(sourceFD, STDIN_FILENO);
                        if(result==-1){
                            fprintf(stderr, "source dup2().\n");
                            fflush(stderr);
                            exit(1);
                            break;
                        }
                        close(sourceFD);
                    }
                    
                    //handle output redirection
                    if(outputFile == NULL) {
                        //redirect standard output to /dev/null for background command
                        if(bg == 'Y' && fg == 0) {
                            int targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0777);
                            if(targetFD==-1){
                                fprintf(stderr, "Failed to open /dev/null.\n");
                                fflush(stderr);
                                exit(1);
                                break;
                            }
                            //redirect stdout, send output to file
                            int result=dup2(targetFD, STDOUT_FILENO);
                            if(result==-1){
                                fprintf(stderr, "target dup2().\n");
                                fflush(stderr);
                                exit(1);
                                break;
                            }
                            close(targetFD);
                        }
                        //do nothing for foreground command 
                    } else if(outputFile != NULL) {
                        //open output file 
                        int targetFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                        if(targetFD==-1){
                            fprintf(stderr, "Failed to open the output file.\n");
                            fflush(stderr);
                            exit(1);
                            break;
                        }
                        //redirect stdout, send output to file
                        int result=dup2(targetFD, STDOUT_FILENO);
                        if(result==-1){
                            fprintf(stderr, "target dup().\n");
                            fflush(stderr);
                            exit(1);
                            break;
                        }
                        close(targetFD);
                    }
                    if(execvp(commandArgs[0], commandArgs)) {
                        fprintf(stderr, "execvp(), invalid command.\n");
                        fflush(stderr);
                        exit(1);
                    }
                    break;
                //parent process
                default: 
                    if(bg == 'Y' && fg == 0) {
                        //do not block if the child process is running in the background
                        printf("background pid is %d\n", childPid);
                        fflush(stdout);
                        //add bg process id to linked list
                        add_id(childPid);
                    } else {
                        //block if child process is running in the foreground
                        waitpid(childPid, &status, 0);
                        
                        //check if the child process terminated abnormally
                        if(WIFSIGNALED(status)) {
                            printf("terminated by signal %d\n", WTERMSIG(status));
                            fflush(stdout);
                        }
                    }
                // monitor background processes
                while((childPid = waitpid(-1, &status, WNOHANG))>0) {
                    printf("background pid %d is done: exit value %d\n", childPid, status);
                    fflush(stdout);
                    //delete bg process id from linked list
                    delete_id(childPid);
                }
            }
        }

        //reset variables before reprompting for command
        totalArgs=0;
        bg = 'N';
        commandLine=NULL;
        for(int i=0; i<519; i++){
            *(commandArgs+i) = 0;
        }
        inputFile = NULL;
        outputFile = NULL;
    }

    printf("Goodbye!\n");
    return 0;
}




