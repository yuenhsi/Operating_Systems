/*
    shell.c
    
    A simple shell that runs basic commands and handles special characters &, <, >, and |
    The EOF command is * in our implementation
    
    Yuen Hsi Chang (with some code from Sherri Goings)
*/

#include    <stdlib.h>
#include    <stdio.h>
#include    <unistd.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/wait.h>
#include    <fcntl.h>
#include    <ctype.h>


char** removeGTandLT(char** words);
char** removeAnd(char** words);
char** readLineOfWords();
char** getCommand();
char* outputFile;
char* inputFile;
int containsAnd = 0;
int pipeCount = 0;
int containsLT = 0;
int containsGT = 0;
int a = 0;

int main()
{
    forkShell();
    return 0;
}

int forkShell() {

    char** wordArray = getCommand();
    if (containsAnd == 1) { // if there is an &, goes here
        wordArray = removeAnd(wordArray);
    }
    if ( (strcmp(wordArray[0], "*") == 0) && (wordArray[1] == NULL) ) {
        printf("Goodbye!\n");
        fflush(stdout);
        exit(0);
    }
    
    a = 0;
    int b = 0;
    int oldDesp[2];  // array of 2 file descriptors
    int newDesp[2];  // another array of 2 file descriptors

    int argumentCount = pipeCount + 1;  // number of commands seperated by pipes
    if (argumentCount != 1) {
        while  (argumentCount != 0) {
            int pid;
            char** pipedArray = (char**) malloc(50 * sizeof(char*));
            while (wordArray[a] != NULL) {
                if (strcmp(wordArray[a], "|") != 0) { 
                    fflush(stdout);
                    pipedArray[b] = wordArray[a];
                    a++;
                    b++;
                }
                else {  // avoids segmentation error
                    if (wordArray[a + 1] == NULL) {   // catches the error when nothing is after |
                        printf("| not followed by a command.\n");
                        fflush(stdout);
                        forkShell();
                    }
                    break;
                }
            }
            b = 0;
            a++;
                                               
            pipe(newDesp);                             // create new pipe
    
            pid = fork();
            if (pid == 0)                  // child process, runs the leftmost command of the pipe
            {
                if (argumentCount == pipeCount + 1) {  // first command
                   close(newDesp[0]);
                   dup2(newDesp[1], 1);
                   close(newDesp[1]);
                }
                else if (argumentCount == 1) {  // last command
                    close(oldDesp[1]);      // not going to overwrite the old output
                    dup2(oldDesp[0], 0);    // accessing the input from process before
                    close(oldDesp[0]);      // close cuz used it
                    close(newDesp[0]);      // not going to read anything from this pipe
                    close(newDesp[1]);      // not going to write anything
                }
                else {  //middle commands
                    close(newDesp[0]);      // not going to get input from newDesp
                    close(oldDesp[1]);      // not going to overwrite old output
                    dup2(oldDesp[0], 0);    // taking in input from "left" pipe
                    dup2(newDesp[1], 1);    // outputting output to "right" pipe
                    close(oldDesp[0]);
                    close(newDesp[1]);
                } 
                executeCommand(pipedArray);
            }
            else  // parent process
            {
                if (argumentCount == pipeCount + 1) {  //first command
                    oldDesp[0] = newDesp[0];
                    oldDesp[1] = newDesp[1];
                }
                else if (argumentCount == 1) {  //last command
                    close(newDesp[1]);
                    close(newDesp[0]);
                    close(oldDesp[1]);
                    close(oldDesp[0]);
                    if (containsAnd == 0) {
                        waitpid(pid, NULL, WUNTRACED);
                    }
                        argumentCount = 0;
                        forkShell();
                }
                else {
                    close(oldDesp[0]);
                    close(oldDesp[1]);
                    oldDesp[0] = newDesp[0];
                    oldDesp[1] = newDesp[1];
                }
            }
            argumentCount = argumentCount - 1;
        }
    }    
    int pid = fork();
    
    if (containsAnd == 0) {
        if (pid == 0)
        {
            executeCommand(wordArray); 
        }
        else
        {
            waitpid(pid, NULL, WUNTRACED);
            forkShell();
        }
    }    
    else {    //if contains &, don't wait
        if (pid == 0)
        {
            executeCommand(wordArray);
        }
        else
        {
            forkShell();
        }
    } 
return pid;   
}


/*  Prompts user for input, and then gets the input.  Calls wordCheck() to see if there are
    any special characters present */
char** getCommand() {
    printf("TurtleShell>> ");
    fflush(stdout);
    char** words = readLineOfWords();

    printf("\n");
    return words;
}

/*  Executes the command that user requested.  Processes the input to remove special characters
    so that it can run the command */
int executeCommand(char** words) {
  // execute command in words[0] with arguments in array words
  // by convention first argument is command itself, last argument must be NULL
    //printf("containsAnd %i\n", containsAnd);
    if (containsGT == 0 && containsLT == 0) {        
        execvp(words[0], words);
     
        // execvp replaces current process so should never get here!
        printf("Not a valid command.  Please try again.\n\n");        
    }
    else { // there is a > or < symbol
        char** newWords = removeGTandLT(words);
        
        // change stdout and/or stdinput
        if (containsGT == 1){       // gets file ready to be written to
            int newfd = open(outputFile, O_CREAT|O_WRONLY, 0644);
            dup2(newfd, 1);
        }
        if (containsLT == 1){       // gets file ready to be read
            int newfd2 = open(inputFile, O_RDONLY, 0644);
            dup2(newfd2, 0);
        }
        execvp(newWords[0], newWords);
            
        // execvp replaces current process so should never get here!
        printf("Not a valid command.  Please try again.\n\n");
        }
    return 0;
}

/*  Removes > and < so that it can run the input */
char** removeGTandLT(char** words) {
    char** newWords = (char**) malloc(50 * sizeof(char*));
    int j = 0;  // counter for words
    int i = 0;  // counter for newWords
    
    // make new array without the > symbol
    while (words[j] != NULL) {
        if (strcmp(words[j], ">") == 0) {
            j = j + 2;
        }        
        else if (strcmp(words[j], "<") == 0) {
            j = j + 2;
        }
        else {
            newWords[i] = words[j];
            j++;
            i++;
        }
    }
    return newWords;
}

/*  Removes & from user input, so that it can run command */
char** removeAnd(char** words) {
    char** newWords = (char**) malloc(50 * sizeof(char*));
    int j = 0;
    
    // make new array without the & at the end
    while (words[j] != NULL) {
        if (words[j+1] == NULL) {
            j = j + 2;
        }
        newWords[j] = words[j];
        j++;
    }
    return newWords;
}

/* 
 * reads a single line from terminal and parses it into an array of tokens/words by 
 * splitting the line on spaces.  Adds NULL as final token 
 */
char** readLineOfWords() {

    // A line may be at most 100 characters long, which means longest word is 100 chars, 
    // and max possible tokens is 51 as must be space between each
    size_t MAX_WORD_LENGTH = 100;
    size_t MAX_NUM_WORDS = 51;
    
    // allocate memory for array of array of characters (list of words)
    char** words = (char**) malloc(MAX_NUM_WORDS * sizeof(char*));
    int i;
    for (i=0; i<MAX_NUM_WORDS; i++) 
    {
        words[i] = (char*) malloc(MAX_WORD_LENGTH);
    }

    // read actual line of input from terminal
    int bytes_read;
    char *buf;
    buf = (char*) malloc(MAX_WORD_LENGTH+1);
    bytes_read = getline(&buf, &MAX_WORD_LENGTH, stdin);
    
    // take each word from line and add it to next spot in list of words
    i=0;
    char* word = (char*) malloc(MAX_WORD_LENGTH);
    word = strtok(buf, " \n");
    fflush(stdout);
    while (word != NULL && i<MAX_NUM_WORDS) 
    {
        strcpy(words[i++], word);
        word = strtok(NULL, " \n");
    }

    // check if we quit because of going over allowed word limit
    if (i == MAX_NUM_WORDS) {
        printf("WARNING: line contains more than %d words!\n", (int)MAX_NUM_WORDS); 
    } 
    else
        words[i] = NULL;
  
    // return the list of words
    checkWords(words);
    return words;
}

/*  Checks for special characters in the user input. */
int checkWords(char** wordList) {
    int i = 0;
    containsAnd = 0;
    pipeCount = 0;
    containsLT = 0;
    containsGT = 0;
    outputFile = "";
    
    while(wordList[i] != NULL)
    {  
        if (strcmp(wordList[i], "|") == 0) {
            pipeCount++;
        }        
        else if (strcmp(wordList[i], "<") == 0) {     
            containsLT = 1;
            if (wordList[i+1] != NULL) {
                inputFile = wordList[i+1];
            }
        }        
        else if (strcmp(wordList[i], ">") == 0) {
            containsGT = 1;            
            if (wordList[i+1] != NULL) {
                outputFile = wordList[i+1];
            }
        }
        else if (strcmp(wordList[i], "&") == 0) {
            if (wordList[i+1] == NULL) {
                containsAnd = 1;
            }
        }
        i++;
    }
    return 0;
}