#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define TRUE ( 1 )
pid_t childpid;

void register_sighandler( int signal_code, void (*handler)(int sig) )
{
    int return_value;
    struct sigaction signal_parameters;
    signal_parameters.sa_handler=handler;
    sigemptyset( &signal_parameters.sa_mask);
    signal_parameters.sa_flags = 0;
    return_value=sigaction( signal_code, &signal_parameters, (void *)0 );

    if( -1 == return_value )
    { 
        perror("sigaction() faild"); 
        exit(1); 
    }
}

void signal_handler( int signal_code )
{
    char * signal_message = "UNKNOWN";
    char * which_process = "UNKNOWN";
    if( SIGINT == signal_code ) signal_message = "SIGINT";
    if( 0== childpid) which_process="Child";
    fprintf (stderr, "%s process (pid %ld) caught signal no. %d (%s)\n",
                which_process, (long int) getpid(), signal_code, signal_message );
}

void cleanup_handler( int signal_code )
{
    char * signal_message = "UNKNOWN"; 
    char * which_process = "UNKNOWN"; 
    if( SIGINT == signal_code ) signal_message = "SIGINT";
    if( 0 == childpid ) which_process = "Child";
    if( childpid > 0 ) which_process = "Parent";
    fprintf( stderr, "%s process (pid %ld) caught signal no. %d (%s)\n",
    which_process, (long int) getpid(), signal_code, signal_message );
    if( childpid > 0 && SIGINT == signal_code )
    {
        int return_value; 
        fprintf( stderr, "Parent (pid %ld) will now kill child (pid %ld)\n",
        (long int) getpid(), (long int) childpid );
        return_value = kill( childpid, SIGKILL ); 
        if( -1 == return_value )
        { perror( "kill() failed" ); exit( 1 ); }
        exit( 0 ); 
    }
}

int main()
{
    childpid = fork();
    if(0==childpid)
    {
        register_sighandler(SIGINT, signal_handler );
        execlp( "ls", "ls");
        printf("this is chlid");
    }
    else
    {
        if( -1== childpid)
        {
            char* errormessage = "UNKNOWN";

            if( EAGAIN == errno ) errormessage = "cannot allocate page table";
            if( ENOMEM == errno ) errormessage = "cannot allocate kernal data";
            fprintf(stderr, "fork() failded becouse : %s\n", errormessage );
            exit(1);
        }

        sleep(1);
        register_sighandler(SIGINT, cleanup_handler);

        printf("this is parr");
        while( TRUE )
        {
            int return_value; 
            fprintf( stderr,
            "Parent (pid %ld) sending SIGINT to child (pid %ld)\n",
            (long int) getpid() , (long int) childpid );
            
            return_value = kill( childpid, SIGINT );
            if( -1 == return_value ) 
            {
                fprintf( stderr,
                "Parent (pid %ld) couldn't interrupt child (pid %ld)\n",
                (long int) getpid(), (long int) childpid );
                perror( "kill() failed" );
                
            }
            sleep(1); 
        }
    }
    exit(0);
}
