// nprocess.cxx by Neil Cooper. See nprocess.h for documentation
#include "nprocess.h"

#include <fstream>      // for ifstream
#include <unistd.h>     // for fork()
#include <stdlib.h>     // for exit()
#include <signal.h>     // for signal() and sighandler_t
#include <dirent.h>     // for opendir()
#include <string.h>     // for strspn() and strlen()
#include <errno.h>      // for errno

#include "nerror.h"     // For EERROR()

using namespace std;


Nprocess::NDAEMON_USER_SIGNAL_HANDLER Nprocess::userSignalHandler = NULL;


void Nprocess::signalHandler( int theSignal )
{
    if ( userSignalHandler )
        userSignalHandler( theSignal );
    else
        {
        LOG( "Daemon got signal ", theSignal, "... exiting" );
        exit( EXIT_FAILURE );
        }
}


void Nprocess::setDaemonSignalHandler( NDAEMON_USER_SIGNAL_HANDLER theHandler )
{
   userSignalHandler = theHandler;
}


void Nprocess::becomeDaemon()
{
    switch( fork() )
        {
        case -1:        // fork() error
            {
            EERROR( "Exiting. Can't fork process for daemon" );
            exit( EXIT_FAILURE );
            break;
            }

        case 0:            // Child (daemon) thread
            {
            setsid();    // Become session/process group leader with no tty
#ifndef DEBUG
            if ( chdir("/") )    // Don't hold any mount points open
               EWARN( "Couldn't change current directory to /" );
#endif
            // Setup a  signal handler so we can exit gracefully.
            // (Default SIGINT behaviour does not call destructors )
            signal( SIGHUP,  signalHandler );    // 1
            signal( SIGINT,  signalHandler );    // 2 (ctrl-C from terminal)
            signal( SIGQUIT, signalHandler );    // 3
            signal( SIGTERM, signalHandler );    // 15

#ifndef DEBUG
            // We won't have a shell any more so close default streams
            close( STDIN_FILENO );
            close( STDOUT_FILENO );
            close( STDERR_FILENO );
#endif
            break;
            }

        default:        // Parent thread
            {
            exit( EXIT_SUCCESS ); // Parent exits and daemon continues
            }
        }
}


pid_t Nprocess::getPid( char* const theProcName )
{
    DIR* procDir = opendir( "/proc" );
    if ( !procDir )
        EERROR( "Can't open /proc directory" );

    bool found = false;
    struct dirent* entry = NULL;
    pid_t pid = 0;

    while ( ( entry = readdir( procDir ) ) && !found )
        {
        if ( strspn( entry->d_name, "0123456789" ) == strlen( entry->d_name ) )
            {
            // We've found a name with all digits meaning its a process
            // now read its name
            ostringstream fname;
            fname << "/proc/" << entry->d_name << "/status";
            ifstream status( fname.str().c_str() );
            if ( status.is_open() )
                {
                string pname;
                status >> pname;
                status >> pname;
                found = ( pname == theProcName );
                if( found )
                    pid = atoi( entry->d_name );
                }
            }
        }

    if( closedir( procDir ) == -1 )
        ERROR( "Can't close /proc directory" );

    return pid;
}


void Nprocess::setOurAffinity( const Nthread::CORE_AFFINITY allowedCores )
{
   unsigned long long flags = allowedCores.getAsInt();
   if ( flags == 0 )
      flags = Nthread::CORE_AFFINITY_ALL.getAsInt();
   cpu_set_t cpus;
   CPU_ZERO( &cpus );
   unsigned short core = 0;

   while ( flags )
      {
      if ( flags & 1 )
         CPU_SET( core, &cpus );
      flags = flags >> 1;
      core++;
      }

    pid_t ourPid = getpid();
    int status = sched_setaffinity( ourPid, sizeof(cpu_set_t), &cpus);
    if ( status )
        NERROR( status, "Nprocess: Could not set affinity for our pid (", ourPid, ")" );
}

