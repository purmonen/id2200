#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

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
        close(fd[0]); 
        dup2(fdin, STDIN_FILENO);
        //dup2(fdin, STDERR_FILENO);
        if (!isLastProgram) {
            dup2(fd[1], STDOUT_FILENO);
        }
        execv(program, args);
    } else {
        int statval;
        wait(&statval);
        if (statval == 512 || statval == 2) {
            printf("Syntax error\n");
            return;
        } else if (statval == 256) {
            printf("Pattern not found\n");
            return;
		} else if (statval == 65280) {
        } else if (statval) {
            printf("Error: %d\n", statval);
            return;
        }

        close(fd[1]); 
        if (!isLastProgram) {
            pipeStuff(fd[0], nextProgramArgs); 
        }
    }
}

void digenv(char *grepArgs[]) {
    char *lessArgs[] = {"/usr/bin/less", NULL};
    char *sortArgs[] = {"/usr/bin/sort", NULL};
    char *printenvArgs[] = {"/usr/bin/printenv", NULL};
    char **programArgs[] = {printenvArgs, grepArgs, sortArgs, lessArgs,  NULL};
    char ***programStart = programArgs;
    if (grepArgs == NULL) {
        programArgs[1] = programArgs[0];
        programStart++;
    }
    pipeStuff(STDIN_FILENO, programStart); 
}

int main(int argc, char *argv[]) {
    argv[0] = "/usr/bin/grep";
    if (argc <= 1) {
        digenv(NULL); 
    } else {
        digenv(argv);
    }
}
