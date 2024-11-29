// nsocket.cxx by Neil Cooper. See nsocket.h for documentation
#include "nsocket.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>	// for inet_ntoa()
#include <fcntl.h>		// for fcntl()
#include <sys/param.h>     // for MAXHOSTNAMELEN
#include <unistd.h>		// for gethostname()
#include <netdb.h>		// for gethostbyname()
#include <string.h>		// for memset()
#include <errno.h>		// for errno

#include "nerror.h"

using namespace std;

namespace NSOCKET
{
// Everything in this namespace is here (rather than in Nsocket )
// so Nsocket::SignalHandler can see it.

// _NSIG is the only way I found of getting the highest signal value
// replace if there turns out to be a better way.
#ifdef __CYGWIN__
    static const int HIGHEST_SIGNAL = NSIG;
#else
    static const int HIGHEST_SIGNAL = _NSIG;
#endif
static bool eventMapInitialised = 0;
static Nmutex eventMapOwner;
static Nevent* eventMap[ HIGHEST_SIGNAL ];
}


void Nsocket::signalHandler(    int         theSignal,
                                siginfo_t*	theSigInfo,
                                void*       theContext      // ucontext_t cast to void*
                                                        )   // static member
{
    NSOCKET::eventMapOwner.lock();
    NSOCKET::eventMap[ theSignal ]->signal();
    NSOCKET::eventMapOwner.unlock();
}


Nsocket::Nsocket(): m_status( CLOSED ),
                    m_notifySignal( -1 ),
                    m_socketReadBuffer( NULL ),
                    m_socketReadBufferSize( 0 ),
                    m_autoBufferThread( NULL ),
                    m_threadDoneUpdate( true ), // true = make it manually resetting
                    m_closePipeCreatedFlag( false )
{
}


Nsocket::~Nsocket()
{
    if ( m_status != CLOSED )
        closeSocket();
}


void Nsocket::listen(   const unsigned short	thePort,
                        const char*				theIpAddress,
                        const unsigned short	theMaxNoOfQueuedConnects )
{
    if ( m_status != CLOSED )
        ERROR("Nsocket::listen: Socket not in closed state");

    char hostName[ MAXHOSTNAMELEN + 1 ];		// Get our own hostname
    gethostname( hostName, MAXHOSTNAMELEN );

    struct hostent* hostInfo = gethostbyname( hostName ); // Get our address info
    if ( !hostInfo )
    switch( h_errno )
        {
        case HOST_NOT_FOUND:
            ERROR( "Nsocket::listen: hostinfo for ourself was not found!" );
            break;

        case NO_ADDRESS:
            // case NO_DATA:  same value as NO_ADDRESS - causes compiler error if uncommented
            ERROR( "Nsocket::listen: hostinfo for ourself does not have an IP address." );
            break;

        case NO_RECOVERY:
            ERROR( "Nsocket::listen: A non-recoverable nameserver error occurred while trying to obtain our own hostinfo." );
            break;

        case TRY_AGAIN:
            ERROR( "Nsocket::listen: Temporary error from nameserver while trying to obtain our own hostinfo." );
            break;

        default:
            ERROR( "Nsocket::listen: Unknown error from gethostbyname while trying to obtain our own hostinfo." );
        }

    m_socket = socket( PF_INET, SOCK_STREAM, 0 );	// Create socket

    struct sockaddr_in sockinfo;
    memset( &sockinfo, 0, sizeof( sockinfo ) );		// Init the structure
    sockinfo.sin_family = hostInfo->h_addrtype;		// this is our host address
    sockinfo.sin_port = htons( thePort );				// this is our port number
    if ( theIpAddress )
        inet_aton( theIpAddress, &( sockinfo.sin_addr ) );	// Optional IP address
    else
        sockinfo.sin_addr.s_addr = htonl( INADDR_ANY );	// Use my IP address

    // Bind address to socket
    int bindStatus = 0;
#ifndef __ANDROID__
     bindStatus =
#endif
     bind( m_socket,( struct sockaddr * )&sockinfo, sizeof( sockinfo ) );
    if ( bindStatus != 0 )
        {
        int e = errno;
        close( m_socket );
        NERROR( e, "Nsocket: Can't bind socket to address '", theIpAddress, "'" );
        }

    // :: Forces compiler to use listen() in the global namespace not Nsocket::listen
    if ( ::listen( m_socket, theMaxNoOfQueuedConnects ) )
        {
        int e = errno;
        close( m_socket );
        NERROR( e, "Nsocket: can't listen on socket" );
        }

    m_status = LISTENING;
}


void Nsocket::accept( Nsocket& theSocket )
{
    if ( theSocket.getStatus() != CLOSED )
        ERROR( "Nsocket::accept: Nsocket given as parameter is not in closed state." );

    if ( m_status != LISTENING )
        ERROR( "Nsocket::accept: Can't accept on a socket that isn't listening." );

    // :: Forces compiler to use accept() in the global namespace not Nsocket::accept
    int socket = ::accept( m_socket, NULL, NULL );
    if ( socket < 0 )
        EERROR( "Nsocket::accept: accept failed." );

    theSocket.m_socket = socket;
    theSocket.m_status = CONNECTED;
}


#ifndef __CYGWIN__
void Nsocket::notifyReady( Nevent* theEvent, int theSignal )
{
    if ( theEvent ) // Set up signalling
        {
        // we can only accept 1 event to notify at a time
        if ( m_notifySignal >= 0 )
            ERROR( "Nsocket::NotifyReady: Notification event already assigned." );

        // Can't do the operaton if the socket is closed
        if ( m_status == CLOSED )
            ERROR( "Nsocket::NotifyReady: Can't be called on socket in closed state." );

        // Acquire the event map
        NSOCKET::eventMapOwner.lock();

        // Initialise it if necessary
        if ( !NSOCKET::eventMapInitialised )
            {
            for (int i = 0; i < NSOCKET::HIGHEST_SIGNAL; i++ )
                NSOCKET::eventMap[ i ] = NULL;
            NSOCKET::eventMapInitialised = true;
            }

        // Check to see signal not already used by another instance of Nsocket
        bool alreadyOwned = ( NSOCKET::eventMap[ theSignal ] != NULL );

        // Associate the event with the signal
        if ( !alreadyOwned )
            NSOCKET::eventMap[ theSignal ] = theEvent;

        // Relinquish control of the event map
        NSOCKET::eventMapOwner.unlock();

        // Check delayed until here so we can return without still
        // owning control of the event map
        if ( alreadyOwned )
            ERROR( "Nsocket::NotifyReady: Requested signal already owned by another instance of an Nsocket." );

        // Set up the signal handler
        struct sigaction  sigAction;
        sigset_t          handlerSigMask;

        if ( sigemptyset( &handlerSigMask ) != 0 )
            EERROR( "Nsocket::NotifyReady: Can't initialise set of signals." );

        sigAction.sa_handler    = NULL;
        sigAction.sa_sigaction  = Nsocket::signalHandler;
        sigAction.sa_mask       = handlerSigMask;
        sigAction.sa_flags      = SA_SIGINFO;

        if ( sigaction( theSignal, &sigAction, NULL ) != 0 )
        EERROR( "Nsocket::NotifyReady: Can't assign action to signal" );

        // Tell the socket which signal to use
        if ( fcntl( m_socket, F_SETSIG, theSignal ) == -1 )
            EERROR( "Nsocket::NotifyReady: Can't assign signal to socket" );

        // Save the signal so we can clear the same one later
        m_notifySignal = theSignal;

        // Tell the socket which process to signal
        if ( fcntl( m_socket, F_SETOWN, getpid() ) == -1 )
            EERROR( "Nsocket::NotifyReady: Can't set process ownership of signal attached to socket" );

        // Save the original flags so we can restore them later
        m_originalNotifyFlags = fcntl( m_socket, F_GETFL );
        if ( m_originalNotifyFlags == -1 )
            {
            int e = errno;
            notifyReady( NULL );
            NERROR( e, "Nsocket::NotifyReady: Can't determine original signal flags from socket" );
            }

        // Tell the socket to start generating signals
        if (    fcntl(	m_socket,
                F_SETFL,
                m_originalNotifyFlags | O_ASYNC ) == -1 )
            {
            int e = errno;
            notifyReady( NULL );
            NERROR( e, "Nsocket::NotifyReady: Can't tell socket to use signals" );
            }
        }
    else	// Cancel signalling
        {
        if ( m_notifySignal < 0 )
            ERROR( "Nsocket::NotifyReady: No signals in use to cancel." );

        // Put back the original flags (stops signals being generated)
        if ( m_originalNotifyFlags != -1 )
            if ( fcntl(	m_socket,
                        F_SETFL,
                        m_originalNotifyFlags ) == -1 )
                EERROR( "Nsocket::NotifyReady: Can't restore original signal flags on socket" );

        // Detach the signal handler
        if ( signal( m_notifySignal, SIG_DFL ) == SIG_ERR )
            EERROR( "Nsocket::NotifyReady: Can't detach signal handler from socket" );

        // Remove the event mapping
        NSOCKET::eventMapOwner.lock();
        NSOCKET::eventMap[ m_notifySignal ] = NULL;
        NSOCKET::eventMapOwner.unlock();

        // m_notifySignal == -1 means signaling not in use
        m_notifySignal = -1;
        }
}
#endif // __CYGWIN__


void Nsocket::connectTo( const unsigned short thePortNum, const char* theHostName )
{
    if ( m_status == REMOTE_CLOSED )
        closeSocket();            // Kill any existing unconnected local connection

    if ( m_status != CLOSED )
        ERROR( "Nsocket::ConnectTo: This socket is not in closed state." );

    struct hostent* hostInfo = gethostbyname( theHostName );
    if ( !hostInfo )
        switch( h_errno )
            {
            case HOST_NOT_FOUND:
                ERROR( "Nsocket::ConnectTo: The specified host is unknown." );
                break;

            case NO_ADDRESS:
                // case NO_DATA:  same value as NO_ADDRESS - causes compiler error if uncommented
                ERROR( "Nsocket::ConnectTo: The specified host is valid but does not have an IP address." );
                break;

            case NO_RECOVERY:
                ERROR( "Nsocket::ConnectTo: A nonrecoverable name server error occurred." );
                break;

            case TRY_AGAIN:
                ERROR( "Nsocket::ConnectTo: A temporary error occurred on anauthoritative name server. Try again later." );
                break;

            default:
                ERROR( "gethostbyname() returned an unknown value: ", h_errno );
            }

    struct sockaddr_in sockinfo;
    memset( &sockinfo, 0, sizeof(sockinfo) );

    memcpy( (char *)&sockinfo.sin_addr, hostInfo->h_addr, hostInfo->h_length );
    sockinfo.sin_family = hostInfo->h_addrtype;
    sockinfo.sin_port = htons( thePortNum );

    m_socket = socket( hostInfo->h_addrtype, SOCK_STREAM, 0 );    // Get socket
    if ( m_socket < 0 )
        EERROR( "Nsocket::ConnectTo: Can't create socket" );

    if ( connect( m_socket, (struct sockaddr *)&sockinfo, sizeof(sockinfo) ) < 0 )
        {
        close ( m_socket );
        EERROR( "Nsocket::ConnectTo: Can't connect socket" );
        }

    m_status = CONNECTED;
}


unsigned long Nsocket::read(    void*				theBuffer,
                                const unsigned long theLength,
                                const bool          theJustReadAvailableFlag,
                                const bool          theBufferReadsFlag,
                                const Ntime         theTimeout,
                                bool*               theTimedOutFlag			)
{
    bool timedOut = false;
    unsigned long length = 0;     // the length we've read

    if ( m_status == LISTENING )  // Prevent misuse
        ERROR( "Nsocket::Read: Attempt to read from socket in listening state" );

    // Update the read buffer here just to do our best to prevent socket locking when full,
    // and also to detect remote closed as early as possible.
    if ( theBufferReadsFlag && (!m_autoBufferThread ) )
        updateReadBuffer();

    unsigned long buffSize = m_readBuffer.size();

    while (	!timedOut                                                                       &&
            ( length < theLength )                                                          &&
            ( theJustReadAvailableFlag ? ( buffSize || socketDataAvailable() ) : true  )    &&
            ( ( m_status == CONNECTED ) || ( ( m_status == REMOTE_CLOSED ) && buffSize ) )      )
        {
        // Fulfill request from the internal read buffer if possible first
        if ( buffSize )
            {
            m_readbufferOwner.lock();

            buffSize = m_readBuffer.size(); // get for sure length now its ours
            while ( ( length < theLength ) && buffSize )
                {
                ( (char*)theBuffer )[length] = m_readBuffer.front();
                m_readBuffer.pop();
                --buffSize;
                ++length;
                }
            // Don't block before the next read if we still have unread data in the internal buffer
            if ( !buffSize )
                m_threadDoneUpdate.unsignal();

            m_readbufferOwner.unlock();
            }

        // If we need more data, read from the socket or just wait if  the thread is running
        if ( ( length < theLength ) && ( !theJustReadAvailableFlag || socketDataAvailable() ) )
            {
            if ( m_autoBufferThread )
                // If the AutoRead thread is running, allow it to fill the buffer
                waitForSocketEvent( &timedOut, theTimeout );
            else
                {
                // AutoRead thread not running, we need to get the data from the socket ourselves.
                if ( theBufferReadsFlag )
                    updateReadBuffer( !theJustReadAvailableFlag, theTimeout, &timedOut );
                else
                    {
                    // We're not buffering
                    unsigned long readLength = 0;

                    rawRead(    &( (char*)theBuffer )[length],
                                theLength - length,
                                &readLength,
                                theJustReadAvailableFlag,
                                theTimeout,
                                &timedOut                       );

                    length += readLength;
                    }
                }
            }
        buffSize = m_readBuffer.size(); // Get latest buffered length before we loop back
        }

    if ( theTimedOutFlag )
        *theTimedOutFlag = timedOut;

    return length;
}


unsigned long Nsocket::write(   const void*         theBuffer,
                                const unsigned long	theBufferLength	)
{
    if ( m_status != CONNECTED )
        return 0;

    unsigned long totalWritten = 0;
    int bytesWritten = 0;

    do
        {
        bytesWritten = send(    m_socket,
                                ( (char*)theBuffer ) + totalWritten,
                                theBufferLength - totalWritten,
                                MSG_NOSIGNAL                        );

        if ( bytesWritten < 0 )
            {
            if ( errno == ECONNRESET )
                m_status = REMOTE_CLOSED;
            }
        else
            totalWritten += bytesWritten;
        }
        while ( ( totalWritten < theBufferLength ) && ( bytesWritten >= 0 ) );    // bytesRead < 0 == network error

    return totalWritten;
}


void Nsocket::closeSocket()
{
    m_status = CLOSED;
#ifndef __CYGWIN__
    // turn off any user-defined signals
    if ( m_notifySignal >=0 )
        notifyReady( NULL );
#endif

    // prevent WaitForRawSocketEvent() from possibly blocking forever on a closed socket.
    if ( m_closePipeCreatedFlag )
        {
        close( m_closePipe[0] );
        close( m_closePipe[1] );
        m_closePipeCreatedFlag = false;
        }

    // Stop autobuffering thread if active
    if ( m_autoBufferThread )
        setAutoReadBuffering( false );

    if ( close( m_socket ) != 0 )
        EERROR( "Nsocket::closeSocket: Can't close socket" );
    else
        {
        m_readbufferOwner.lock();
        while (!m_readBuffer.empty())
            m_readBuffer.pop();
        m_readbufferOwner.unlock();

        if ( m_socketReadBuffer )
            {
            m_socketReadbufferOwner.lock();
            delete m_socketReadBuffer;
            m_socketReadBuffer = NULL;
            m_socketReadbufferOwner.unlock();
            }
        }
}


void Nsocket::getRemoteName(char* theBuffer, unsigned long theLength )
{
    if ( m_status != CONNECTED )
        ERROR( "Nsocket::GetRemoteName: Socket not in connected state." );

    sockaddr_in name;
    socklen_t size = sizeof( name );

    if ( getpeername( m_socket, (sockaddr*)&name, &size ) < 0 )
        EERROR( "Nsocket::GetRemoteName: getpeername() failed." );

    if ( theLength >= size )
        strncpy( theBuffer, inet_ntoa( name.sin_addr ), theLength ); // struct in_addr
    else
        ERROR( "Nsocket::GetRemoteName: Buffer provided is too small to contain name." );
}


unsigned short Nsocket::GetRemotePort()
{
    if ( m_status != CONNECTED )
        ERROR( "Nsocket::GetRemotePort: Socket not in connected state." );

    sockaddr_in name;
    socklen_t size = sizeof( name );

    if ( getpeername( m_socket, (sockaddr*)&name, &size ) < 0 )
        EERROR( "Nsocket::GetRemotePort: getpeername() failed." );

    return ntohs( name.sin_port );
}


Nsocket::NSOCKET_STATUS Nsocket::getStatus()
{
    return m_status;
}


bool Nsocket::acceptIsAvailable()
{
    if ( m_status != LISTENING )
        ERROR( "Nsocket::AcceptIsAvailable: Socket not in listening state." );

    struct pollfd ufd;

    ufd.fd = m_socket;
    ufd.events = POLLIN | POLLPRI;
    ufd.revents = 0;

    int status = poll( &ufd, 1, 0 );
    if ( status == -1 )
        EERROR( "Nsocket::AcceptIsAvailable: poll() call failed." );

    return ( status > 0 );
}


void Nsocket::waitForSocketEvent( bool* theTimedOutFlag, const Ntime theTimeout )
{
    if ( m_status == CLOSED )
        ERROR( "Nsocket::WaitForSocketEvent: Socket is in closed state." );

    bool timedOut = false;

    if ( m_readBuffer.size() == 0 ) // only block if we dont have buffered data to read
        if ( m_autoBufferThread )
            timedOut = !m_threadDoneUpdate.wait( theTimeout );
        else
            waitForRawSocketEvent( &timedOut, theTimeout );

    if ( theTimedOutFlag )
        *theTimedOutFlag = timedOut;
}


bool Nsocket::readWillNotBlock()
{
    return ( ( m_readBuffer.size() > 0 ) || socketDataAvailable() );
}


bool Nsocket::writeWillNotBlock()
{
    if ( m_status != CONNECTED )
        ERROR( "Nsocket::WriteWillNotBlock: Socket not in connected state" );

    struct pollfd ufd;

    ufd.fd = m_socket;
    ufd.events = POLLOUT;
    ufd.revents = 0;

    int retVal = poll( &ufd, 1, 0 );
    if ( retVal == -1 )
        EERROR( "Nsocket::WriteWillNotBlock: poll() failed" );

    return ( retVal > 0 );
}


void Nsocket::setKeepAlives( bool theEnableFlag )
{
    if ( m_status != CONNECTED )
        ERROR( "Nsocket::SetKeepAlives: Socket not in connected state" );

    int opt = theEnableFlag ? 1 : 0;

    if ( setsockopt( m_socket, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof( opt ) ) == -1 )
        EERROR( "Nsocket::SetKeepAlives: Can't set keepalive option on socket" );
}


void Nsocket::setAutoReadBuffering( const bool theEnableFlag )
{
    if ( theEnableFlag && ( m_status != CONNECTED ) ) // allow closeSocket() to disable auto buffering
        ERROR( "Nsocket::SetAutoReadBuffering: Socket not in connected state." );

    // Only allow one thread and dont allow disabling a non-existent thread
    if ( theEnableFlag ? ( m_autoBufferThread != NULL ) : ( m_autoBufferThread == NULL ) )
        ERROR( "Nsocket::SetAutoReadBuffering: Already in requested state." );

    if ( theEnableFlag )
        {
        m_threadDoneUpdate.unsignal();
        m_autoBufferThread = new Nthread( autoBufferProc, this );
        }
    else
        {
        m_autoBufferThreadStop.signal();
        m_autoBufferThread->getReturnValue();
        delete ( m_autoBufferThread );
        m_autoBufferThread = NULL;
        m_threadDoneUpdate.signal();  // Prevent any other threads blocking forever
        }
}


unsigned long Nsocket::getBufferedDataLength()
{
    return m_readBuffer.size();
}


// ---------- PRIVATE METHODS --------------

void* Nsocket::autoBufferProc( void* theParam )
{
    Nsocket& us = *(Nsocket*)theParam;

    while ( !us.m_autoBufferThreadStop.currentState() )
        {
        us.updateReadBuffer( true );  // true = wait for data
        us.m_threadDoneUpdate.signal();
        }
    us.m_autoBufferThreadStop.unsignal();

    return NULL;
}


// Read data from socket.
// Return value is false only in the event of a network/socket error.
// If the remote end closes cleanly, RawRead still returns true but sets status to REMOTE_CLOSED.
// If JustReadAvailable is true, timeouts are obviously irrelvant/ignored.
void Nsocket::rawRead(  void*			    theBuffer,
                        const unsigned long theLength,
                        unsigned long*	    theReadCount,
                        const bool          theJustReadAvailableFlag,
                        const Ntime&        theTimeout,
                        bool*               theTimedOutFlag             )

{
    unsigned long  length = 0;
    bool timedOut = false;

    // Wait until we read enough or timeout or connection dropped or network error
    while (	( length < theLength )      &&
            !timedOut                   &&
            ( m_status == CONNECTED )   &&
            ( !theJustReadAvailableFlag || socketDataAvailable() ) )
        {
        if ( !( theJustReadAvailableFlag || socketDataAvailable() ) )
            waitForRawSocketEvent( &timedOut, theTimeout );

        long readLength = 0;
        if ( !timedOut )
            {
            readLength = recv(  m_socket,
                                ( (char*)theBuffer ) + length,
                                theLength - length,
                                0 );
            if ( readLength > 0 )
                length += readLength;
            else
                {
                // man page says recv returns 0 == peer orderly shutdown -1 == network error
                m_status = REMOTE_CLOSED;
                // Next 2 lines disabled because remote windows sockets seem to cause readLength -1 when closing
                //  if ( readLength < 0 )
                //    status = false;
                }
            }
        }

    if ( theTimedOutFlag )
        *theTimedOutFlag = timedOut;

    if ( theReadCount )
        *theReadCount = length;
}


bool Nsocket::socketDataAvailable()
{
    if ( m_status != CONNECTED )
        return false;

    struct pollfd ufd;

    ufd.fd = m_socket;
    ufd.events = POLLIN | POLLPRI;
    ufd.revents = 0;

    return ( poll( &ufd, 1, 0 ) > 0 );
}


// Create the temporary buffer used only by UpdateReadBuffer
void Nsocket::createSocketReadBuffer()
{
    if ( m_socketReadBuffer )
        ERROR( "Nsocket::CreateSocketReadBuffer: Buffer already created." );

    m_socketReadbufferOwner.lock();

    // Read the size of the sockets incoming data buffer
    if ( !m_socketReadBufferSize )
        {
        socklen_t returnValLength = sizeof( m_socketReadBufferSize );
        int retVal = getsockopt( m_socket, SOL_SOCKET, SO_RCVBUF, &m_socketReadBufferSize, &returnValLength );
        if ( retVal == -1 )
            EERROR( "Nsocket::CreateSocketReadBuffer: Can't read socket buffer size" );
        if ( m_socketReadBufferSize <= 0 )
            ERROR( "Nsocket::CreateSocketReadBuffer: socket buffer size reported is ", m_socketReadBufferSize );
        }

    m_socketReadBuffer = new char[ m_socketReadBufferSize ];
    if ( !m_socketReadBuffer )
        EERROR( "Nsocket::CreateSocketReadBuffer: Can't create socket read buffer" );

    m_socketReadbufferOwner.unlock();
}


// Wait for socket event from socket itself.
// NB:  It doesn't differentiate between remote end closing and data arriving
void Nsocket::waitForRawSocketEvent( bool* theTimedOutFlag, const Ntime theTimeout )
{
    if ( !m_closePipeCreatedFlag )
        createClosePipe();

    struct pollfd ufds[2];

    ufds[0].fd = m_socket;
    ufds[0].events = POLLIN | POLLPRI;
    ufds[0].revents = 0;
    ufds[1].fd = m_closePipe[0];
    ufds[1].events = POLLIN | POLLPRI;
    ufds[1].revents = 0;

    long long timeout = theTimeout.getAsMs();
    if ( timeout > INT_MAX )
        timeout = INT_MAX;    // Best we can do. Its still a long wait :-)

    if (timeout == 0 )
        timeout = -1;

    int retVal = 0;
    do
        {
        retVal = poll( ufds, 2, timeout );
        } while ( ( retVal == -1 ) && ( errno == EINTR ) ); // ignore failures because of signals

    if ( retVal == -1 )
        EERROR( "Nsocket::WaitForRawSocketEvent: Can't poll socket" );

    bool timedOut = ( retVal == 0 );

    if ( theTimedOutFlag )
        *theTimedOutFlag = timedOut;
}


// Read whatever data is available from the socket and put it in the read buffer.
// If theWaitForDataFlag is true, we'll wait for the given timeout (0=infinite) for something to arrive (of any length).
void Nsocket::updateReadBuffer( const bool theWaitForDataFlag, const Ntime& theTimeout, bool* theTimedOutFlag )
{
    bool timedOut = false;

    if ( !m_socketReadBuffer )
        createSocketReadBuffer();

    unsigned long length = 0;

    if ( theWaitForDataFlag )
        while ( ( m_status == CONNECTED ) && ( !length ) && ( !timedOut ) )
            {
            waitForRawSocketEvent( &timedOut, theTimeout );
            rawRead( m_socketReadBuffer, m_socketReadBufferSize, &length, true );
            }
    else
        rawRead( m_socketReadBuffer, m_socketReadBufferSize, &length, true );

    if ( length )
        {
        m_readbufferOwner.lock();
        for (unsigned long i=0; i < length; i++ )
            m_readBuffer.push( m_socketReadBuffer[i] );
        m_readbufferOwner.unlock();
        }

    if ( theTimedOutFlag )
        *theTimedOutFlag = timedOut;
}


bool Nsocket::createClosePipe()
// In order to stop a call to WaitOnRawSocketEvent() locking up for ever if the socket gets closed
// meanwhile by another thread, we need to make a mechanism whereby close() can abort the wait.
// The wait actually polls 2 file descriptors, the socket itself and one end of a pipe. So this
// allows Close() to terminate the wait by sending an arbitarary byte down the pipe. Actually
// I've discvered that just closing the pipe is enough, so we don't even have to send a byte.
{
    bool status = ( !m_closePipeCreatedFlag );
    if ( status )
        {
        status = ( pipe( m_closePipe ) == 0 );
        m_closePipeCreatedFlag = status;
        }
    return status;
}
