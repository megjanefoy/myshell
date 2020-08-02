#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

/*
 * Title: Assignment 3
 * Semester: COP4338 - Spring 2021
 * Author Megan Jane Thompson
 *
 * This program creates a unix shell that handles commands, pipelines
 * and I/O redirections.
 */

#define MAX_ARGS 20															//maximum number of commands
#define MAX_BUFF 1024														//maximum length of command


int shouldWait;																//global flag for async or not


/* This method takes in an input filename as a string, opens
 * the file as read only, and alters IO accordingly.
 *
 * @param inFileName	,string of chars being the filename
 */
void redirectInput(char *inFileName){
    int inFile = open(inFileName, O_RDONLY);								//open input file as read only

    if (inFile < 0){														//error opening file
    	printf("Opening input filename was unsuccessful. Program is terminating.\n");
    	exit(1);
    }
    else{																	//success opening file
    	dup2(inFile, 0);
    }

    close(inFile);															//close input file
}


/* This method takes in an output filename as a string, opens
 * the file as write only or creates the file if the file does
 * not exist. It then alters IO accordingly.
 *
 * @param outFileName	,string of chars being the filename
 * @param redirectType	,int 1 overwrites and int 2 appends
 */
void redirectOutput(char *outFileName, int redirectType){
    int outFile;

    if(redirectType == 1){
    	outFile = open(outFileName, O_WRONLY | O_TRUNC  | O_CREAT, 0600); 	//open output file either overwrite or create
    }
    else if(redirectType == 2){
    	outFile = open(outFileName, O_WRONLY | O_APPEND | O_CREAT, 0600);	//open output file either append or create
    }

    if (outFile < 0){														//error opening file
    	printf("Opening output filename was unsuccessful. Program is terminating.\n");
    	exit(1);
    }
    else{																	//success opening file
    	dup2(outFile, 1);
    }
    close(outFile);															//close output file
}


/* This method takes in a string of arguments and
 * executes the commands.
 *
 * @param args	,string of arguments to be executed
 */
void executeCmds(char *args[]){
	pid_t cpid;
	cpid = fork();															//create child process
	if (cpid < 0){
		printf("Fork() was unsuccessful. Program is terminating.\n");
		exit(1);
	}
	else if (cpid == 0){ 													//Child Process
		execvp(args[0], args);												//call to execute command
	}
	else{ 																	//Parent Process
		if (shouldWait){
			waitpid(cpid, NULL, 0);											//parent to wait on child process
		}
	}
	redirectInput("/dev/tty");												//redirect input
	redirectOutput("/dev/tty", 1);											//redirect output
}


/* This method takes in a string of arguments and
 * creates an IO pipe accordingly.
 *
 * @param args	,string of remaining arguments
 */
void runPipe(char *args[]) {
    int pipefd[2];
    pipe(pipefd);															//opens pipe

    dup2(pipefd[1], 1);														//make output go to pipe
    close(pipefd[1]);

    executeCmds(args);														//calls executeCmds() to processes remaining args

    dup2(pipefd[0], 0);														//make input go to pipe
    close(pipefd[0]);
}


/* This method takes in a string of commands and
 * and array of arguments. It uses strtok() to separate
 * the commands and store them into the array.
 *
 * @param cmds			,string of commands to be separated
 * @param numArgsArr	,array of arguments to hold commands
 * @return				,an int representing number of commands
 */
int getArgs(char *cmds, char *numArgsArr[]){
	int i = 0;

	numArgsArr[0] = strtok(cmds, "\n\t ");									//extract first command
	++i;

	if(numArgsArr[0] == NULL || numArgsArr[0] == "\n"){						//check if null or new line
		return 0;
	}

	while((numArgsArr[i] = strtok(NULL, "\n\t ")) != NULL){					//extract remaining commands
		if(i >= MAX_ARGS){
			printf("Too many arguments. Max of 20 allowed. Program terminating.\n");
			exit(1);
		}
	}

	return i;
}


/* This method takes in the user's command line to be processed
 * and sent to corresponding methods.
 *
 * @param cmdline		,string of commands to be processed
 */
void processCmd(char *cmdline){
	int i;
	char* args[MAX_ARGS];
	char* numArgsArr[MAX_ARGS];
	char cmds[strlen(cmdline)+1];

	strcpy(cmds, cmdline);													//copies user's command line

	if (cmds[strlen(cmds) - 1] == '&') {									//checks to see if async or sync
		shouldWait = 0;														//flags if parent should not wait
		cmds[strlen(cmds) - 1] = '\0';										//replaces & with \0
		cmdline[strlen(cmds) - 1] = '\0';
	}
	else{
		shouldWait = 1;														//flags if parent should wait
	}

	int numargs = getArgs(cmds, numArgsArr);								//obtains number of commands
	if(numargs <= 0){														//checks if no arguments
		printf("No arguments provided. Program is terminating.\n");
		exit(1);
	}

	if(!strcmp(numArgsArr[0], "quit") || !strcmp(numArgsArr[0], "exit")){	//checks program should stop
		exit(0);
	}

	char *word = strtok(cmdline, " ");										//extracts first command
	i = 0;
	while (word != NULL) {
		if (*word == '<') {													//redirect input if command is '<'
			redirectInput(strtok(NULL, " "));
		}
		else if (!strcmp((char*)word, ">")) {								//redirect output to overwrite if command is '>'
			redirectOutput(strtok(NULL, " "), 1);
		}
		else if (!strcmp((char*)word, ">>")){								//redirect output to append if command is '>>'
			redirectOutput(strtok(NULL, " "), 2);
		}
		else if (*word == '|') {											//create pipe if command is '|'
			args[i] = NULL;
			runPipe(args);													//call to runPipe()
			i = 0;
		}
		else {
			args[i] = word;													//if not special command
			i++;
		}
		word = strtok(NULL, " ");											//get next command
	}
	args[i] = NULL;

	executeCmds(args);														//call to executeCmds()
}


/*
 * Main
 */
int main (int argc, char *argv []){
	char cmdline[MAX_BUFF];

	for(;;){																//keep looping until terminated
		printf("COP4338$ ");
		fflush(stdout);
		if(fgets(cmdline, MAX_BUFF, stdin) == NULL){						//get user input and save to cmdline
			perror("fgets failed");
			exit(1);
		}
		size_t ln = strlen(cmdline)-1;
		if (cmdline[ln] == '\n'){											//remove new line character from input
			cmdline[ln] = '\0';
		}
		processCmd(cmdline);												//call to start processing input
	}
	return 0;
}
