/* $begin shellmain */
#include "csapp.h"
#include "csapp.c"
#include <errno.h>
#define MAXARGS   128

char* current = NULL;
char* prev = NULL;

/* Function prototypes */
void eval(char *cmdline, int* fd1, int* fd2);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void pipe_execute(char* cmdline);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */


    while (1) {
       /* Read */
       printf("CSE4100-SP-P2> ");                   
       fgets(cmdline, MAXLINE, stdin); 
       if (feof(stdin)) exit(0);

       /* Evaluate */
       if(!strchr(cmdline, '|')) eval(cmdline, NULL, NULL);
        else pipe_execute(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline, int* fd1, int* fd2) 
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
            if(fd1 && !fd2){
                close(fd1[0]);
                Dup2(fd1[1], 1);
            }
            else if(fd1 && fd2){
                close(fd1[1]);
                close(fd2[0]);
                Dup2(fd1[0], 0);
                Dup2(fd2[1], 1);
            }
            else if(!fd1 && fd2){
                close(fd2[1]);
                Dup2(fd2[0], 0);
            }

            if(execvp(argv[0], argv) < 0){
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }

        if(fd1 && !fd2) close(fd1[1]);
        else if(fd1 && fd2){
            close(fd1[0]);
            close(fd2[1]);
        }
        else if(!fd1 && fd2) close(fd2[0]);

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

/* $begin pipe */
void pipe_execute(char* cmdline){
    int cnt = 0;
    char* tmp = strtok(cmdline, "|");
    char** cmd_token = malloc(sizeof(int*) * MAXARGS);
    
    //실행해야할 프로세스 개수 세기
    while(tmp != NULL){
        cmd_token[cnt] = tmp;
        cnt++;
        tmp = strtok(NULL, "|");
    }

    int* fd1 = malloc(sizeof(int) * 2);
    int* fd2 = malloc(sizeof(int) * 2);
    pipe(fd1);
    pipe(fd2);

    eval(cmd_token[0], fd1, NULL);

    for(int i = 1; i < cnt-1; i++){
        eval(cmd_token[i], fd1, fd2);
        free(fd1);
        fd1 = malloc(sizeof(int) * 2);
        fd1[0] = fd2[0];
        fd1[1] = fd2[1];

        free(fd2);
        fd2 = malloc(sizeof(int) * 2);
        pipe(fd2);
    }

    eval(cmd_token[cnt-1], NULL, fd1);
    
    free(fd1);
    free(fd2);
    
}
/* $end pipe */


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
    int idx = 0;

    while ((delim = strchr(buf, ' '))) {
        if(*buf == '"'){
            char* tmp = strchr(buf+1, '"');
            argv[argc++] = ++buf;
            *tmp = '\0';
            buf = tmp + 1;            
        }
       else {
            argv[argc++] = buf;
           *delim = '\0';
           buf = delim + 1;
        }
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