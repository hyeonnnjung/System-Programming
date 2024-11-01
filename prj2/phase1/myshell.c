/* $begin shellmain */
#include "csapp.h"
#include "csapp.c"
#define MAXARGS   128

char* current = NULL;
char* prev = NULL;

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

int main() 
{
    char cmdline[MAXLINE]; /* Command line */


    while (1) {
       /* Read */
       printf("CSE4100-SP-P2> ");                   
       fgets(cmdline, MAXLINE, stdin); 
       if (feof(stdin)) exit(0);

       /* Evaluate */
       eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 
    if (argv[0] == NULL)  
       return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        // if (execve(argv[0], argv, environ) < 0) {   //ex) /bin/ls ls -al &
        //     printf("%s: Command not found.\n", argv[0]);
        //     exit(0);
        // }
        if((pid = Fork()) == 0){
            if(execvp(argv[0], argv) < 0){
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }

       /* Parent waits for foreground job to terminate */
       if (!bg){ 
           int status;
            pid = Waitpid(pid, &status, 0);
       }
       else//when there is backgrount process!
           printf("%d %s", pid, cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "quit")) /* quit command */
       exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
       return 1;

    else if(!strcmp(argv[0], "cd")){
        if(argv[1] == NULL || !strcmp(argv[1], "~") || !strcmp(argv[1], "~/")) {
            prev = getcwd(NULL, 1000);
            chdir(getenv("HOME"));
            current = getcwd(NULL, 1000);
        }
        else if(argv[1][0] == 126 && argv[1][1] == 47 && argv[1][2] != 0){
            char* tmp = getcwd(NULL, 1000);
            chdir(getenv("HOME"));
            char* path_tmp = malloc(sizeof(char) * 1000);
            int i = 2;
            while(argv[1][i] != 0){
                path_tmp[i-2] = argv[1][i];
                i++;
            }
            path_tmp[i-2] = '\0';
            int check = chdir(path_tmp);

            char* home = getenv("HOME");
            char* home_str = strdup(home);
            char* combine = malloc(strlen(home_str) + strlen(path_tmp) + 1);

            strcpy(combine, home_str);
            strcat(combine, "/");
            strcat(combine, path_tmp);
            if(check < 0){
                printf("bash: cd: %s: No such file or directory\n", combine);
                return 1;
            }
            prev = tmp;
            current = getcwd(NULL, 1000);
        }
        else if(!strcmp(argv[1], "-")){
            if(prev == NULL) printf("bash: cd: OLDPWD not set\n");
            else{
                chdir(prev);
                char* tmp = prev;
                prev = current;
                current = tmp;
            }
        }
        else {
            char* tmp = getcwd(NULL, 1000);
            int check = chdir(argv[1]);
            if(check < 0){
                printf("bash: cd: %s: No such file or directory\n", argv[1]);
                return 1;
            }
            prev = tmp;
            current = getcwd(NULL, 1000);
        }
        return 1;
    }

    else if(!strcmp(argv[0], "exit")){
        exit(0);
    }
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
       buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
       argv[argc++] = buf;
       *delim = '\0';
       buf = delim + 1;
       while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
       return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
       argv[--argc] = NULL;

    return bg;
}
/* $end parseline */