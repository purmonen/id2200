#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void pipeStuff(int fdin, char ***programArgs) {
    char *program = **programArgs;
    char **args = *programArgs;
    char ***nextProgramArgs = programArgs+1;
    int isLastProgram = *(nextProgramArgs) == NULL;

    int fd[2];
    pipe(fd);

    int childpid = fork();
    if (childpid == -1) {
        printf("Could not fork!\n");
        return;
    }
    if (!childpid) {
        if (close(fd[0])) {
            printf("Close did not work!\n");
        }
        if (dup2(fdin, STDIN_FILENO) < 0) {
            printf("dup2 fail!\n");
            return;
        }
        if (dup2(fdin, STDERR_FILENO) < 0) {
            printf("dup2 fail!\n");
            return;
        }
        //dup2(fdin, STDERR_FILENO);
        if (!isLastProgram) {
            if (dup2(fd[1], STDOUT_FILENO) < 0) {
                printf("dup2 fail!\n");
                return;
            }
        }
        execvp(program, args);
    } else {
        int statval;
        
        if(!wait(&statval)) {
            printf("Wait failed!\n");
        }
        
        // Grep - syntax error codes
        if (statval == 512 || statval == 2) {
            printf("Syntax error\n");
            return;
        } else if (statval == 256) {
            printf("Pattern not found\n");
            return;
        } else if (statval) {
            printf("Error: %d\n", statval);
            return;
        }

        if(close(fd[1])) {
            printf("Close did not work!\n");
        }
        
        if (!isLastProgram) {
            pipeStuff(fd[0], nextProgramArgs); 
        }
    }
}

void digenv(char *grepArgs[]) {
    char *lessArgs[] = {"less", NULL};
    char *sortArgs[] = {"sort", NULL};
    char *printenvArgs[] = {"printenv", NULL};
    char **programArgs[] = {printenvArgs, grepArgs, sortArgs, lessArgs,  NULL};
    char ***programStart = programArgs;
    if (grepArgs == NULL) {
        programArgs[1] = programArgs[0];
        programStart++;
    }
    pipeStuff(STDIN_FILENO, programStart); 
}

int main(int argc, char *argv[]) {
    argv[0] = "grep";
    if (argc <= 1) {
        digenv(NULL); 
    } else {
        digenv(argv);
    }
    return 0;
}
