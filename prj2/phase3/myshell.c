/* $begin shellmain */
#include "csapp.h"
#include "csapp.c"
#include <errno.h>
#define MAXARGS   128

char* current = NULL;
char* prev = NULL;
pid_t shell_pid;
volatile int jobs_count = 1;
volatile pid_t pid_reaped = 0;
volatile pid_t pid_reaped2 = 0;

/* Function prototypes */
void eval(char *cmdline, int* fd1, int* fd2);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void pipe_execute(char* cmdline);
void sigchld_handler(int signal);
int parseline2(char *buf, char **argv);


struct job{
    int status; // 0 = terminate, 1 = suspended, 2 = running
    pid_t pid;
    int job_index;
    char command[100];
};

struct job jobs[1000];

void sigint_handler(){
    Sio_puts("\n");
    Sio_puts("CSE4100-SP-P2> ");
}

void sigtstp_handler(){
    Sio_puts("\n");
    Sio_puts("CSE4100-SP-P2> ");
}

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    shell_pid = Getpgrp();

    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGTTOU, SIG_IGN);
    Signal(SIGTSTP, sigtstp_handler);
    Signal(SIGINT, sigint_handler);

    while (1) {
       /* Read */
       printf("CSE4100-SP-P2> ");                   
       fgets(cmdline, MAXLINE, stdin); 
       if (feof(stdin)) exit(0);

       /* Evaluate */
       if(!strchr(cmdline, '|')) eval(cmdline, NULL, NULL);
        else {
            pipe_execute(cmdline);
        }
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
    if (!builtin_command(argv)) { 
        //quit -> exit(0), & -> ignore, other -> run
        // if (execve(argv[0], argv, environ) < 0) {   //ex) /bin/ls ls -al &
        //     printf("%s: Command not found.\n", argv[0]);
        //     exit(0);
        // }
        if((pid = Fork()) == 0){
            setpgid(0, 0);

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
                jobs[jobs_count].pid = pid;
                jobs_count++;
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
            jobs[0].status = 2;
            jobs[0].pid = pid;
            //printf("eval함수에서 pid : %d\n", pid);
            char command_tmp[100] = "";
            strcat(command_tmp, argv[0]);
            int i = 1;
            while(argv[i] != NULL){
                strcat(command_tmp, " ");
                strcat(command_tmp, argv[i]);
                i++;
            }

            strcpy(jobs[0].command, command_tmp);

            tcsetpgrp(STDIN_FILENO, pid);

            //여기에 자식 프로세스 종료를 기다리는 로직 추가
            //뭔가 sigprocmask도 사용해야할 것 같은데 일단은 패스하도록 하자...
            //reaping할 pid는 어디서 발생해야 하는가... -> SIGCHLD
            //waitpid는 handler에서 처리

            while(pid != pid_reaped){
                sigset_t mask;
                Sigemptyset(&mask);
                Sigsuspend(&mask);
            }

            tcsetpgrp(STDIN_FILENO, shell_pid);
       }
       else{
            //when there is backgrount process!
           //printf("%d %s", pid, cmdline);
            if(!fd1 && !fd2){
                jobs[jobs_count].status = 2;
                jobs[jobs_count].pid = pid;
                jobs[jobs_count].job_index = jobs_count;

                char command_tmp[100] = "";
                strcat(command_tmp, argv[0]);
                int i = 1;
                while(argv[i] != NULL){
                    strcat(command_tmp, " ");
                    strcat(command_tmp, argv[i]);
                    i++;
                }

                strcpy(jobs[jobs_count].command, command_tmp);
                jobs_count++;
            }
        }
    }
    return;
}

/* $begin pipe */
void pipe_execute(char* cmdline){
    char original_cmdline[1000] = "";
    strcpy(original_cmdline, cmdline);

    char *argv[MAXARGS];
    char buf[MAXLINE];
    strcpy(buf, cmdline);
    int bg = parseline2(buf, argv);
    if(bg){
        jobs[jobs_count].status = 2;
        jobs[jobs_count].job_index = jobs_count + 1;

        char command_tmp[100] = "";
        strcat(command_tmp, argv[0]);
        int i = 1;
        while(argv[i] != NULL){
            strcat(command_tmp, " ");
            strcat(command_tmp, argv[i]);
            i++;
        }

        strcpy(jobs[jobs_count].command, command_tmp);
    }

    int cnt = 0;
    char* tmp = strtok(cmdline, "|");
    char** cmd_token = malloc(sizeof(int*) * MAXARGS);
    
    //실행해야할 프로세스 개수 세기
    while(tmp != NULL){
        cmd_token[cnt] = tmp;
        cnt++;
        tmp = strtok(NULL, "|");
    }

    if(bg){
        for(int i = 0 ; i < cnt; i++){
            if(i != cnt - 1){
                int new_length = strlen(cmd_token[i]) + 2;
                char* new_cmd_token = (char*)malloc(new_length);
                strcpy(new_cmd_token, cmd_token[i]);
                strcat(new_cmd_token, " &");
                cmd_token[i] = new_cmd_token;
            }
        }
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

    else if(!strcmp(argv[0], "jobs")){
        for(int i = 1; i < jobs_count; i++){
            char status_print[50];
            if(jobs[i].status == 1){
                strcpy(status_print, "suspended");
                printf("[%d] %s %s\n", jobs[i].job_index, status_print, jobs[i].command);
            }
            else if(jobs[i].status == 2){
                strcpy(status_print, "running");
                printf("[%d] %s %s\n", jobs[i].job_index, status_print, jobs[i].command);
            }
        }
        return 1;
    }

    else if(!strcmp(argv[0], "kill") && argv[1][0] == '%'){
        int flag = 0; //현재 인자로 들어온 프로세스 인덱스가 잘못된 인덱스라면 = 0
        int i;
        if(argv[1][0] == '%'){
            int idx = atoi(argv[1]+1);
            for(i = 1; i < jobs_count; i++){
                if(jobs[i].job_index == idx && jobs[i].status != 0){
                    jobs[i].status = 0;
                    flag = 1;
                    break;
                }
            }
        }

        if(flag){
            kill(jobs[i].pid, SIGKILL);
        }
        else{
            printf("No Such Job\n");
        }
        
        return 1;
    }

    else if(!strcmp(argv[0], "bg")){
        int flag = 0; //현재 인자로 들어온 프로세스 인덱스가 잘못된 인덱스라면 = 0
        int i;
        if(argv[1][0] == '%'){
            int idx = atoi(argv[1]+1);
            for(i = 1; i < jobs_count; i++){
                if(jobs[i].job_index == idx && jobs[i].status == 1){
                    
                    jobs[i].status = 2;
                    flag = 1;
                    char status_print[20] = "running";
                    printf("[%d] %s %s\n", jobs[i].job_index, status_print, jobs[i].command);
                    break;
                }
            }
        }

        if(flag){
            Kill(jobs[i].pid, SIGCONT);
        }
        else{
            printf("No Such Job\n");
        }
        
        return 1;
    }

    else if(!strcmp(argv[0], "fg")){
        int flag = 0; //현재 인자로 들어온 프로세스 인덱스가 잘못된 인덱스라면 = 0
        int i;
        if(argv[1][0] == '%'){
            int idx = atoi(argv[1]+1);
            for(i = 1; i < jobs_count; i++){
                if(jobs[i].job_index == idx && jobs[i].status != 0){
                    jobs[i].status = 0;
                    flag = 1;
                    char status_print[20] = "running";
                    printf("[%d] %s %s\n", jobs[i].job_index, status_print, jobs[i].command);
                    break;
                }
            }
        }

        if(flag){
            jobs[i].status = 0;

            jobs[0].status = 2;
            jobs[0].pid = jobs[i].pid;
            //printf("here : %d\n", jobs[0].pid);

            strcpy(jobs[0].command, jobs[i].command);
            //printf("jobs[0] : %s\n", jobs[0].command);

            //printf("kill 하는 pid : %d\n", jobs[i].pid);
            kill(jobs[i].pid, SIGCONT);
            tcsetpgrp(STDIN_FILENO, getpgid(jobs[i].pid));
            

            while(jobs[i].pid != pid_reaped){
                sigset_t mask;
                Sigemptyset(&mask);
                Sigsuspend(&mask);
            }

            tcsetpgrp(STDIN_FILENO, shell_pid);
        }
        else{
            printf("No Such Job\n");
        }
        
        return 1;
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

void sigchld_handler(int signal){
    pid_t pid;
    int status;

    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigprocmask(SIG_SETMASK, &mask, &prev);
    
    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
        pid_reaped = pid;
        //printf("jobs[0] pid : %d\n", jobs[0].pid);
        //printf("reaped_pid : %d\n", pid_reaped);
        //suspended된 경우
        if(WIFSTOPPED(status)){
            if(pid == jobs[0].pid){
                jobs[jobs_count].status = 1;
                jobs[jobs_count].pid = jobs[0].pid;
                jobs[jobs_count].job_index = jobs_count;
                strcpy(jobs[jobs_count].command, jobs[0].command);
                jobs_count++;
            }
            else{
                for(int i = 1; i < jobs_count; i++){
                    if(pid == jobs[i].pid){
                        jobs[i].status = 1;
                        break;
                    }
                }
            }
        }
        //terminated된 경우
        else{
            for(int i = 1; i < jobs_count; i++){
                if(pid == jobs[i].pid){
                    jobs[i].status = 0;
                    break;
                }
            }
            pid_reaped2 = pid;
        }
    }
    
    Sigprocmask(SIG_SETMASK, &prev, NULL);
}

int parseline2(char *buf, char **argv) 
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