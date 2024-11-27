#ifndef NTCPSERVER_H
#define NTCPSERVER_H

#include <map>
#include "nsocket.h"
#include "nthread.h"

//  NtcpServer v1.0 by Neil Cooper 22nd April 2005
//  Implements a generic multi-threaded TCP service provider object.
//  The user just needs to provide a function that will be automatically run
//  by each client thread, one per incoming TCP connection.
//  This function must be of type NTCPSERVER_THREAD_PROC.
//  NOTES on the design of the user-supplied function:
//  The CLIENT_PARAMS struct passed as a parameter by the server contains an already
//  opened/connected socket to the remote client, and the optional user-provided
//  parameter passed in to the Start method.
//  As a means of communicating to the user-function that either the destructor or
//  Stop() method was called on the NtcpServer instance, the server will close the
//  socket that was passed as a parameter. It will also wait for the user-function to exit.
//  The user-supplied function should therefore be written in such a way that it does
//  not block on waiting for socket data to arrive by just calling Nsocket::Read(), as
//  this call does not return if the socket is concurrently closed by another thread.
//  The Nsocket::WaitForSocketEvent() should be used to block, then any available data
//  can be read/processed until the socket is closed by the server.
//  Note that it is not necesary to explicitly close the socket in the user-function
//  prior to exiting.


class NtcpServer
{
public:

    // Structure of the parameter passed to the user-supplied client thread process
    typedef struct
        {
        Nsocket clientSocket; // Worker thread's own connection to client.
        void*   userParam;    // User-supplied parameter
        } CLIENT_PARAMS;

    // User-supplied per-client thread process
    typedef void ( *NTCPSERVER_THREAD_PROC )( CLIENT_PARAMS& );


    NtcpServer( NTCPSERVER_THREAD_PROC  theClientProcess,       // User-supplied client function
                const unsigned short    thePort,                // TCP port to listen on
                const char*             theIpAddress = NULL,    // Optional parameter for which NIC to use
                void*                   theUserParam = NULL );  // Optional parameter to pass to client threads.
    //  Constructs and immediately starts a TCP server according to the given parameters.
    //  Parameters:
    //    theClientProcess: User-supplied client function run for each icnoming connection
    //    thePort:          TCP port to listen on
    //    theIpAddress:     Optional IP address for multi-node systems indicating whic NIC to use.
    //    theUserParam:     Optional user-parameter to pass to client threads as CLIENT_PARAMS.userParam.
    //  NB. This constructor does not return until an error occurs, or the server terminates following another
    //  thread calling Stop().


    NtcpServer();

    virtual ~NtcpServer();


    void start( NTCPSERVER_THREAD_PROC  theClientProcess,
                const unsigned short    thePort,
                const char*             theIpAddress = NULL,
                void*                   theUserParam = NULL );
    //  (Re)Starts the TCP server according to the given parameters.
    //  This method is provided for use with instances created with the default contructor.
    //  NB. This method does not return until the server terminates following another
    //  thread calling Stop().
    //  Parameters:
    //    theClientProcess: User-supplied client function run for each icnoming connection
    //    thePort:          TCP port to listen on
    //    theIpAddress:     Optional IP address for multi-node systems indicating whic NIC to use.
    //    theUserParam:     Optional user-parameter to pass to client threads as CLIENT_PARAMS.userParam.


    bool stop();
    //  Signals a running server to stop. This method may return before the server has actally stopped.
    //  the compelx constructor or call to Start() will only return after the server has actually stopped.
    //  return: true = success, false = fail ( server was not running, already stopping, or other internal error ).

    private:
    typedef struct
        {
        NtcpServer*    us;             // Placeholder for this
        Nthread*       clientThread;   // Client thread object
        CLIENT_PARAMS  params;         // Client thread parameters.
        bool           imDoneFlag;     // indicator that thread is ending
        } THREAD_CONTEXT;

    Nsocket                         m_serverSocket;
    Nmutex                          m_serverSocketOwner;
    std::vector<THREAD_CONTEXT*>    m_clientList;
    Nmutex                          m_clientListOwner;
    Nevent                          m_collectGarbage;
    NTCPSERVER_THREAD_PROC          m_clientProcess;
    Nthread*                        m_garbageCollector;


    static void  safeGarbageCollector( void* theParam );
    static void* garbageCollector( void* theParam );
    static void  safeClientThread( void* theParam );
    static void* clientThread( void* theParam );
};

#endif
