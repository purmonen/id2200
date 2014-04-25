/* NAME
 *   digenv - a program that displays filtered environment variables
 *
 * SYNOPSIS
 *   ./digenv [grepargs]   
 *
 * DESCRIPTION
 *   This program lists sorted environment variables in stdout and allows
 *   filtering using grep. If called with arguments it's logically equivalent
 *   to running the following in shell:
 *     printenv | sort | grep args | less
 *
 * OTIONS
 *   Exactly the same as for grep since all arguments are sent to grep
 *
 *   Without args it's the same as:
 *     printenv | sort | less 
 *
 * AUTHOR
 *   Sami Purmonen, purmonen@kth.se
 *   Robin Tillman, robint@kth.se
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

/** executeProgramPipeline
 *   
 * executeProgramPipeline returns 0 if executing the pipeline 
 * of programs succeeded else 0
 *
 * Def:
 *   executes each program connecting each programs stdout to the next
 *   programs stdin if the next programs is not the last program:
 *     program1 | program2 ... | programN
 */
int
executeProgramPipeline (
    int   fdin,            /* Input source to the current program */
    char  ***programs)     /* null-terminated list of programs */ 
{
    char **program = *programs;
    char *programName = *program;
    char ***nextPrograms = programs+1;
    int isLastProgram = *(nextPrograms) == NULL;

    int fd[2];
    if (pipe(fd)) {
        printf("Could not pipe");
        return 1;
    }

    int childpid = fork();
    if (childpid == -1) {
        printf("Could not fork!\n");
        return 1;
    }
    if (!childpid) {
        if (close(fd[0])) {
            printf("Close did not work!\n");
        }
        if (dup2(fdin, STDIN_FILENO) < 0) {
            printf("dup2 fail!\n");
            return 1;
        }
        /* Redirect stderr so that error output from intermediary programs are hidden */
        if (dup2(fdin, STDERR_FILENO) < 0) {
            printf("dup2 fail!\n");
            return 1;
        }
        /* The last programs output must go to STDOUT */
        if (!isLastProgram) {
            if (dup2(fd[1], STDOUT_FILENO) < 0) {
                printf("dup2 fail!\n");
                return 1;
            }
        }
        execvp(programName, program);
    } else {
        int statval;
        
        if(!wait(&statval)) {
            printf("Wait failed!\n");
            return 1;
        }
        
        /* Display more useful error output when grep fails */
        if (strcmp(programName, "grep") == 0) {
            if (statval == 512 || statval == 2 || statval == 1) {
                printf("Syntax error\n");
                return 1;
            }
            if (statval == 256) {
                printf("Pattern not found\n");
                return 1;
            }
        } else if (statval) {
            printf("Error: %d\n", statval);
            return 1;
        }
        if(close(fd[1])) {
            printf("Close did not work!\n");
        }
        if (!isLastProgram) {
            return executeProgramPipeline(fd[0], nextPrograms); 
        }
    }
    return 0;
}

/** main
 *
 *  main returns 0 if success or 1 on error
 *
 *  Def:
 *   if (args)
 *     printenv | sort | grep args | less
 *   else
 *     printenv | sort | less 
 */
int
main(
    int   argc,       /* number of args to grep */
    char  *grep[])    /* All arguments sent to the program goes to grep */
{
    /* Define all programs to be sent to executeProgramPipeline */
    grep[0] = "grep";
    char *less[] = {"less", NULL};
    char *more[] = {"more", NULL};
    char *sort[] = {"sort", NULL};
    char *printenv[] = {"printenv", NULL};

    /* If the user has a pager set in env use that - otherwise default to less */
    char *envPager = getenv("PAGER");
    char **pager = envPager != NULL && strcmp(envPager, "MORE") == 0 ? more : less;

    char **programs[] = {printenv, grep, sort, pager,  NULL};

    /* If no arguments were provided skip the grep progam */
    char ***programStart = programs;
    if (argc <= 1) {
        programs[1] = programs[0];
        programStart++;
    }

    // Standard input is the input source to the first program
    return executeProgramPipeline(STDIN_FILENO, programStart); 
}
