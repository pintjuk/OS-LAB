#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500 

#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

/* bool stuff */
#define true ( 1 )
#define false ( 0 )
typedef int bool;

/* id of current forground process */
pid_t f_proc=-1;

/* maximum length of a comand or an argument */
#define WLEN 30
/* maximum length of lien to read */
#define LLEN 200
/* maximum amount of arguments */
#define MAXW 20

/* for pipes */
#define PIPE_READ_SIDE ( 0 )
#define PIPE_WRITE_SIDE ( 1 )

FILE *fmemopen(void *buf, size_t size, const char *mode);

/* macro for error checking and printing error messages
 *
 * retval:  return value form a function
 * msg:     message to print in case of error
 */
#define CHECKERR(retval, msg) if( -1 == retval) { perror( msg ); fprintf(stderr, "line:%i \n", __LINE__); exit( 1); }

/* creates a pipe and checks for errors
 */
void mkpipe(int* p_pipe)
{
    int return_value = pipe( p_pipe );
    assert(p_pipe!=NULL);
    CHECKERR(return_value, "cannot create pipe"); 
}

/* closes a file descriptor and checks for errors
 */
void closedisk(int disk){
    int return_value=close(disk);
    assert(disk>=0);
    CHECKERR(return_value, "cannot close file discriptor");
}

/* checks the status of terminated processes
 */
void checkProcStat(int status){
    pid_t childpid=0;
    if( WIFEXITED( status ) )
    {
        int child_status = WEXITSTATUS( status );
        if( 0 != child_status )
        {
            fprintf( stderr, "Child (pid %ld) failed with exit code %d\n",
            (long int) childpid, child_status );
        }
    }
    else
    {
        if( WIFSIGNALED( status ) ) 
        {
            int child_signal = WTERMSIG( status );
            fprintf( stderr, "Child (pid %ld) was terminated by signal no. %d\n",
            (long int) childpid, child_signal );
        }
    }
}

/* converts integer to a string
 *
 * str:     pointer to where the string should be wrighten
 * len:     length of the alocate space for the strings
 * val:     integer to converts
 * ofset:   the ofset from str where minigful text starts
 *          will be wrighen to where this points 
 */
void getDecStr (char* str, const int len, int val, int* ofset)
{
  unsigned int i;
  bool neg=false;
  if(val <0){
      val = (~val)+1;
      neg=true;
  }
  for(i=2; i<=len; i++)
  {
    str[len-i] = (char) ((val % 10) + '0');
    val/=10;
  }
  for(i=0; i<len; i++)
  {
    if ('0'==str[i])
    {
        str[i]=' ';
    }
    else
    {
        if(i-1>0)
        {
            if(neg)
                str[i-1]='-';
            (*ofset)=i-1;
        }
        else
        {
            if(neg)
                str[0]='-';
            (*ofset)=0;
        }
        break;
    }
  }
  str[len-1] = '\0';
}

/* interupt handler for SIGINT for the main process
 */
void inthandler(int sig){
    if(f_proc!=-1)
    {
        kill(SIGINT, f_proc);
    }
    else
    {
        kill(SIGINT, 0);
        exit(0);
    }
}

/* interput handler for SIGCHLD for the main process
 */
void chldhandler(int sig){
    int pid;
    char buffer [12];
    int ofset=0;
    while ( (pid=waitpid(-1, NULL, WNOHANG)) > 0){
        write(STDOUT_FILENO, "\nBackgroud process ", 19);
        getDecStr(buffer, 12, pid, &ofset);
        write(STDOUT_FILENO, buffer+ofset, 12-ofset);
        write(STDOUT_FILENO, " termenated!\n", 14);
    }
}

/* wait for process x to termenate while blocking
 *
 * x:   pid of te process to wait for
 */
void waitfor(const unsigned int x){
    int waitresult;
    int status;
    while(-1==(waitresult=waitpid(x, &status, 0)))
    {
        if(ECHILD==errno)
        {
            break;
        }
        if(EINTR==errno)
        {
            continue;
        }
        CHECKERR(waitresult, "faild waitpid()");
    }
    checkProcStat(status);
}

/* Help function that gegesters an interput handler
 *
 * signal_code:     code of the signal
 * sig:             signal handler function pointer
 * flags:             flags
 */
void register_sighandler( int signal_code, void (*handler)(int sig), int flags )
{
    int return_value;
    struct sigaction signal_parameters;
    signal_parameters.sa_handler=handler;
    sigemptyset( &signal_parameters.sa_mask);
    signal_parameters.sa_flags = flags;
    return_value=sigaction( signal_code, &signal_parameters, (void *)0 );

    if( -1 == return_value )
    { 
        perror("sigaction() faild"); 
        exit(1); 
    }
}

/* reads one line from stdin and splits words at white spaces
 *
 * vwords:  where to whright comands and arguments
 *          chould point to word
 */
int readline(char** vwords,int* wc){
    int next_char=0;
    int iword=0;
    int ichar=0;
    bool quotemode=false;
    bool escape=false;
    (*wc)=0;
  
    while(true)
    {
        next_char=fgetc(stdin);
        if((' ' == (char) next_char ||
            EOF == next_char ||
           '\n' == (char) next_char) && !quotemode)
        {
            if(ichar>0)
            {
                vwords[iword][ichar]='\0';
                ichar=0;
                (*wc)++;
                iword++;
            }
            if('\n'== (char) next_char)
            {
                vwords[iword]=NULL;
                return 0;
            }
            if(EOF == next_char)
            {
                vwords[iword]=NULL;
                return 1;
            }
        }
        else if('\\'== (char) next_char&&!escape)
        {
            escape=true;
        }
        else if('\"'== (char) next_char&&!escape)
        {
            quotemode=!quotemode;
        }
        else
        {
            vwords[iword][ichar]=(char) next_char;
            ichar++;
            escape=false;
        }

        if(iword >= (MAXW-1))
        {
            vwords[MAXW-1]=NULL;
            return 0;
        }
        if(ichar >= WLEN)
        {
            vwords[iword][WLEN-1]='\0';
            iword++;
            ichar=0;
            continue;
        }
    }
}

/*
pipe configuration for execchild
*/
struct fileDisc
{
    
    int pipeIn;
    int pipeOut;
    int* toClose;
};

/*
Runs vwords in a child process

vwords:		a list containing the command and arguments
bg:		wether to block until command termenates
pipes:		pipe configuration
altcomand: 	comands to run should the main command fail 
*/
int execchild(char* vwords[], bool bg,  struct fileDisc* pipes, char* altcomands[]){
    char errmsg[100];
    time_t oldtime;
    time_t newtime;
    int i;
    pid_t childpid;
    childpid = fork();
    if(0==childpid){
        if(pipes!=NULL)
        {
            if(pipes->pipeIn!=-1)
            {
                int return_value= dup2( pipes->pipeIn,STDIN_FILENO);
                CHECKERR(return_value, "cannot dup");
            }
            if(pipes->pipeOut!=-1)
            {

                int return_value;
                return_value= dup2( pipes->pipeOut, STDOUT_FILENO);
                CHECKERR(return_value, "cannot dup");
            }
            i=0;
            while(pipes->toClose[i]!=-1){
                closedisk(pipes->toClose[i]);
                i++;
            }
        }

        execvp(vwords[0], vwords);
        /*TODO: fix this buffer overflow*/
        /* WTF, ANSI sucks*/
        sprintf(errmsg, "faild to run %s ", vwords[0]);
        perror(errmsg);
        i=0;
        if(altcomands!=NULL)
            while(altcomands[i]!=NULL)
            {
                char* temp[]={NULL, NULL};
                temp[0]=altcomands[i];
                execvp(altcomands[i], temp);
                sprintf(errmsg, "faild to run %s", altcomands[i]);
                perror(errmsg);
                i++;
            }
        exit(1);
    } 
    CHECKERR(childpid, "faild fork()");

    if(!bg){
        f_proc=childpid;
        time(&oldtime);
        waitfor(childpid);
        time(&newtime);
        printf("\nprocess %i terminated after %f seconds\n", childpid, difftime(newtime, oldtime));
        f_proc=-1;
        return 0;
    }
    else
    {
        return childpid;
    }
    return -1;
}

/*
Displays enviroment variables in a pager,

vwords should be a pointer to an array of MAXW pointers to char strings with WLEN elements,
and contain the comand that was passed by the user.
*/
void checkEnv(char** vwords){
    int pipe1[ 2 ]; 
    int pipe2[ 2 ];
    int pipe3[ 2 ];
    int child1;
    int child2;
    int child3;
    int child4=-1;
    int pipes[7];
    char* morecomand="more";
    char* altcomands[]={NULL, NULL};
    char* pager=getenv("PAGER");
    struct fileDisc fd_pipes;
    char* command[2];
    bool dogrep=false;
    altcomands[0]=morecomand;
    if(vwords[1]!=NULL)
        dogrep=true;

    
    command[1]=NULL;
    
    mkpipe(pipe1);
    mkpipe(pipe2);
    mkpipe(pipe3);
    
    pipes[0] = pipe1[0];
    pipes[1] = pipe1[1];
    pipes[2] = pipe2[0];
    pipes[3] = pipe2[1];
    pipes[4] = pipe3[0];
    pipes[5] = pipe3[1];
    pipes[6] = -1;
    fd_pipes.toClose = pipes;

    fd_pipes.pipeIn  = -1;
    if( dogrep)
        fd_pipes.pipeOut = pipe3[PIPE_WRITE_SIDE];
    else
        fd_pipes.pipeOut = pipe1[PIPE_WRITE_SIDE]; 
    command[0]  = "printenv";
    child1= execchild(command, true, &fd_pipes, NULL);

    if( dogrep)
    {
        fd_pipes.pipeIn = pipe3[PIPE_READ_SIDE];
        fd_pipes.pipeOut= pipe1[PIPE_WRITE_SIDE];
        vwords[0] = "grep";
        child4 = execchild(vwords, true, &fd_pipes, NULL);
    }
    
    fd_pipes.pipeIn = pipe1[PIPE_READ_SIDE];
    fd_pipes.pipeOut= pipe2[PIPE_WRITE_SIDE];
    command[0]= "sort"; 
    child2= execchild(command, true, &fd_pipes, NULL);
   
    fd_pipes.pipeIn = pipe2[PIPE_READ_SIDE];
    fd_pipes.pipeOut= -1;
    if(pager!=NULL)
    {
        command[0] = pager;
        child3= execchild(command, true, &fd_pipes, NULL);
    }
    else
    {
        command[0] = "less";
        child3= execchild(command, true, &fd_pipes, altcomands);
    }
    
    closedisk(pipe1[PIPE_READ_SIDE]);    
    closedisk(pipe1[PIPE_WRITE_SIDE]);
    closedisk(pipe2[PIPE_READ_SIDE]);
    closedisk(pipe2[PIPE_WRITE_SIDE]);
    closedisk(pipe3[PIPE_READ_SIDE]);
    closedisk(pipe3[PIPE_WRITE_SIDE]);

    waitfor(child1);
    if( dogrep )
        waitfor(child4);
    waitfor(child2);
    waitfor(child3);
}
/*
reaps all childes in zombie state, does not block
*/
void reap_all(){
    int status;
    int pid;
    while((pid=waitpid(-1, &status, WNOHANG))){
        if(pid!=-1)
        {
            printf("Backgroud process %i Terminated!\n", pid);
            continue;
        }

        if(ECHILD==errno)
        {
            break;
        }

        else if (EINTR==errno)
        {
            continue;
        }

        else
        {
            perror("waitpid() faild");
            exit(1);
            break;
        }
    }
}

void cleanexit()
{
    int return_value=kill(0, SIGINT);
    CHECKERR(return_value, "Faild kill(0, SIGINT)");
}

int main()
{
    int return_value;
   register_sighandler(SIGINT, inthandler, 0);
#ifdef SIGDET
   register_sighandler(SIGCHLD, chldhandler,SA_RESTART|SA_NOCLDSTOP);
#endif
    while(!feof(stdin))
    {
        char cwdbuf[100];
        char words[MAXW][WLEN];
        char* vwords[MAXW];
        int wc;
        int i;
        for(i=0; i<MAXW; i++)
        {
            vwords[i]=words[i];
        }

#ifndef SIGDET
        reap_all();
#endif
        getcwd(cwdbuf, sizeof(cwdbuf));
        printf("%s > ",cwdbuf);

        if(readline(vwords, &wc)){
            printf("\nReached EOF\n");
            cleanexit();
        }
        if(wc==0)
        {
            continue;
        }
        if(!strcmp(vwords[0], "exit"))
        {
            cleanexit();  
            return 1;
        }
        if(!strcmp(vwords[0], "cd"))
        {
            return_value=chdir(vwords[1]); 
            CHECKERR(return_value, "Faild chdir");
            continue;
        }
        if(!strcmp(vwords[0], "checkEnv"))
        {
            checkEnv(vwords);
            continue;
        }
        if(!strcmp(vwords[wc-1], "&")){
            vwords[wc-1]=NULL;
            execchild(vwords, true, NULL, NULL);
            continue;
        }
        execchild(vwords, false, NULL,  NULL); 
    }
    return 0;
}
