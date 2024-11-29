#ifndef NPROCESS_H
#define NPROCESS_H

#include "nthread.h"

// Nprocess v1.4 by Neil Cooper 6th December 2019
// Implements a simple interface to create and maintain processes.

class Nprocess
{
public:

   typedef void ( *NDAEMON_USER_SIGNAL_HANDLER )( int );
//  type of function that will run as a user-specified signal handler

   static void setDaemonSignalHandler( NDAEMON_USER_SIGNAL_HANDLER theHandler );
//  Allows the user to define their own signal handler that will be
//  called for the following termination signals sent to the dameon
//  created by a call to BecomeDameon:
//  SIGHUP  (1)
//  SIGINT  (2) (ctrl-C from terminal)
//  SIGQUIT (3)
//  SIGTERM (15)
//  If no user-supplied handler is provided, a default one is provided
//  that calls exit() to ensure that all destructors are called in the
//  event of any of the above signals being raised.
//  NB By experimentation, this is not default behaviour (at least for SIGINT).
//  parameter:
//      Pointer to user-created signal handler.

   static void becomeDaemon();
//  Intended to be called by the main thread of a process.
//  BecomeDaemon() converts the caller into a daemon process.

   static pid_t getPid( char* const theProcName );
//  Returns the pid of the first running process found with the given name,
//  or returns 0 if no process with the given name is running.

#ifndef __ANDROID__
   static void setOurAffinity( const Nthread::CORE_AFFINITY allowedCores = Nthread::CORE_AFFINITY_ALL );
//  Set core affinity for the calling process. 0 = All cores.
#endif

private:
   static NDAEMON_USER_SIGNAL_HANDLER userSignalHandler;
   static void signalHandler( int theSignal );
};

#endif
