#ifndef NSOCKET_H
#define NSOCKET_H

// Nsocket v1.5 by Neil Cooper 11th Nov 2014
// Implements a client or server TCP socket connection.
// Unless otherwise specified, all methods returning bool: true=success, false=fail.
// Fail indicates either:
// * An unrecoverable internal system error has occurred.
// * An unknown/unrecoverable network error has occurred.
// * Call failed because we are in the wrong state for the requested operation. ( See GetStatus() ).

#include <sys/poll.h>
#include <signal.h>
#include <queue>
#include "nevent.h"
#include "nmutex.h"
#include "ntime.h"
#include "nthread.h"


class Nsocket
{
public:
	// Status return from GetStatus
	enum NSOCKET_STATUS	{
								CLOSED,         // Socket is closed.
								LISTENING,      // Socket is accepting incoming connections following successful call to Listen()
								CONNECTED,      // A client connection exists between this socket and a remote socket.
								REMOTE_CLOSED   // The remote socket closed or link broke after CONNECTED state was achieved.
								};

	Nsocket();
	virtual ~Nsocket();


	// --- SERVER MODE METHODS ---

	void listen(    const unsigned short  thePort,
					const char*           theIpAddress = NULL,
				    const unsigned short  theMaxNoOfQueuedConnects = 0 );
	//  Puts the socket in LISTENING state and accepts incoming connections.
	//  Calls to Listen() will fail unless the socket is in the CLOSED state.
	//  Parameters:
	//    thePort:      IP Port number to listen on.
	//    theIpAddress: Optional IP address to listen on (for systems with more than one NIC)
	//    theMaxNoOfQueuedConnects: Upto this many remote connection attempts can be
	//      queued up before the remote end obtains an immediate fail from a subsequent
	//      connection attempt.

	void accept( Nsocket& theSocket );
	//  If successful, the NSocket passed in as a parameter will acquire the connection.
	//  Its state will be CONNECTED and should subsequently be used to access/manage the connection.
	//  The called instance will continue to remain in LISTENING state.
	//  NB. The call will fail if the state of the called socket is not LISTENING or the
	// state of the Nsocket passed in is not CLOSED.
	//  Parameter:
	//      Nsocket object that will acquire the client connection.

	bool acceptIsAvailable();
	//  Returns true only if socket status is LISTENING and an incoming connection is available.
	// (i.e. Accept() will not block).


	// --- CLIENT MODE METHODS ---

	void connectTo( const unsigned short thePort, const char* theHostName = "localhost" );
	//  Attempts to connect to an existing (server mode) socket.
	//  Parameters:
	//      theHostName: Hostname or IP address of machine to connect to.
	//      thePort: IP Port to connect to.


	// -- CLIENT OR SERVER MODE METHODS ---

	void getRemoteName(char* theBuffer, unsigned long theLength );
	//  Returns the IP address of the connected peer (remote) socket.
	//  Parameters:
	//      theBuffer: Pointer to buffer to receive the data
	//      theLength: Length of buffer pointed to by theBuffer

	unsigned short GetRemotePort();
	//  Returns the TCP port number of the remote (connected) client or server.

	unsigned long read(	void*               theBuffer,
                        const unsigned long theLength,
                        const bool          theJustReadAvailableFlag = false,
                        const bool          theBufferReadsFlag = false,
                        const Ntime         theTimeout = (long)0,
                        bool*               theTimedOutFlag = NULL              );
	//  Reads data sent by the remote socket. If JustReadAvailableFlag = true, it will read any
	//  buffered data without blocking, otherwise this call will block until at least the requested
	//  number of bytes have been received, the optional (inter-byte) timeout passes, or a network
	//  event occurs, such as the remote end closing.
	//  If the remote end closes before enough data is received to fill the request, Read
	//  will return true, but theReadCount will be less than theLength and a call to GetStatus()
	//  will return REMOTE_CLOSED.
	//  NB: When using unbuffered reads, the remote end closing cannot be detected until all
	//  available data has been read. This is a limitation in the design of BSD sockets generally.
	//  To avoid this, use buffered reads or set AutoReadBuffering.
	//  Parameters:
	//      theBuffer: Pointer to buffer to receive the data
	//      theLength: Length of data to read, or maximum length if theJustReadAvailableFlag is set.
	//      theJustReadAvailableFlag:   true  = Don't block and just return any available data
	//                                  false = Block until theLength bytes have been received
	//                                          or remote end diconnects.
	//      theBufferReadsFlag:         true  = Read whole blocks and buffer internally. This
	//                                          minimises the number of system calls when doing
	//                                          small reads. If AutoReadBuffering has been enabled
	//                                          then this parameter is ignored.
	//                                  false = Don't buffer data internally (do 'raw' reads).
	//      theTimeout:   Optional maximum time between received bytes before timeout occurs. Note
	//                    that this is NOT a timeout for the whole block to arrive. 0 = no timeout.
	//      theTimeoutFlag: Optional pointer to variable to be set to true if a timeout occurrs.
	//	Return value:
	//		No. of bytes received.
	// NB. The remote end closing properly is not a socket error. To test for this, call GetStatus();

	unsigned long write( const void* theBuffer, const unsigned long theBufferLength );
	//  Sends the given data to the remote socket, and will block until the write operation is
	//  complete. NB: The data may be buffered. Returning from Write() does not imply the remote
	//  end has read the data.
	//  Parameters:
	//      theBuffer = Pointer to data to send.
	//      theBufferLength = Size of data to send.
	//  Return value:
	//      No.of bytes sent. Returned value < theBufferLength indicates a socket error occurred.

	void closeSocket();
	//  Closes the open socket. Also frees any internally used resources such as threads and signal handlers.

	NSOCKET_STATUS getStatus();
	//  Returns the current state of the socket (see NSOCKET_STATUS enum).
	//  NB: BSD sockets are designed such that the remote end closing can only be detected by
	//  attempting an I/O operation on a socket. This means that the status from
	//  GetStatus() is the status follwing the last call that performed an actual IO operation
	//  on the socket. This means you can't just e.g poll GetStatus() to detect when the remote end
	//  closes, unless AutoReadBuffering is set, as this performs threaded socket I/O internally.

#ifndef __CYGWIN__
	void notifyReady( Nevent* theEvent, int theSignal = SIGIO );
	//  Once called, Nsocket will asynchronously signal given event when either incoming
	//  data or an incoming connection is detected. The exact behaviour depends on the
	//  current state of the Nsocket as returned by GetStatus() when NotifyReady() is
	//  called. NotifyReady() will fail if the current state of the Nsocket is anything
	//  other than LISTENING or CONNECTED.
	//
	//  If the current state is LISTENING, (i.e. the socket is a server-mode socket)
	//  the given event will subsequently be signalled by incoming connections.
	//  i.e. The event can be used to ensure calls to Accept() will not block.
	//  If the current state is CONNECTED (i.e. the socket is a client-mode socket)
	//  the given event will be signalled by incoming data.
	//  i.e. The event can be used to ensure calls to Read() of 1 byte will not block.
	//    Note that only a single signal may occur even if multiple bytes arrive to be read.
	//
	//  To cancel NotifyReady behaviour, call NotifyReady again with theEvent parameter
	//  of NULL. (TheSignal parameter is ignored in this form of the call).
	//
	//  NB. The OS internally notifies the Nsocket object of an incoming data/connection
	//  with the use of signals. By default, the signal used is SIGIO however in case
	//  other signal schemes are also in use, you may specify another signal to use by
	//  providing its enumerated value (in signal.h) as the 2nd parameter.
	//
	//  Linux signals are process-wide, therefore if you wish to set NotifyReady() on more
	//  than one instance of an Nsocket concurrently, unique signals should be specified for
	//  each instance. NotifyReady will detect any attempt at shared signal usage with other
	//  instances of Nsocket and will explicitly fail in this case.
	//
	//  Parameters:
	//      theEvent = The event to signal when an incoming connection is detected.
	//      theSignal = The signal to use internally to detect an incoming connection.
#endif

	void waitForSocketEvent( bool* theTimedOutFlag = NULL, const Ntime theTimeout = (long)0 );
	//  This method will only return when one of the following occur:
	//  * The optional timeout has occurred. (The default value of 0 prevents timeouts).
	//  * The socket was Close()d by another thread after this method was called.
	//  * if listening: a connection has been received (Accept() will not block)
	//  * if not listening: a 1 byte read will not block (readable data exists or remote end closed).
	//  Parameter:
	//      theTimedOutFlag = pointer to boolean to set to true if timeout occurs before event.
	//      theTimeout      = amount of time to wait before returning. Default is infinite.

	bool readWillNotBlock();
	//  Returns true only if there is data available or the remote end has closed.
	//  (i.e. an ubuffered read of 1 byte will not block). The only way to differentiate between
	// data being available or the remote end having closed is to actually do a read. This is a
	// limitation of BSD sockets.

	bool writeWillNotBlock();
	//  Returns true if socket write buffer is not full (i.e. a write of 1 byte will not block).

	void setKeepAlives( const bool theEnableFlag );
	//  When SetKeepAlives has been turned on, a "keepalive" probe packet is sent after a period
	//  of inactivity (i.e. when no data has passed either way over the socket) to see if the remote
	//  end is still there. Unfortunately in Linux, the duration of inactivity is about 2 hours by
	//  default, and it is a system-wide kernel configuration parameter (see man tcp(7) ).
	//  In the event that a probe fails a certain number of times, the socket signals SIGPIPE,
	//  therefore if you enable keepalives you also need to have already set up a signal handler
	//  for SIGPIPE otherwise the app will die when a keepalive fails. (An object of type Nsocket
	//  does not have keep-alives enabled when created).
	//  Parameter:
	//      theEnableFlag: true = enable, false = disable.
	//  This call will fail if the current state is not CONNECTED.

	void setAutoReadBuffering( const bool theEnableFlag );
	//  If AutoReadBuffering is enabled (default: disabled), a thread is started that continually
	//  tranfers any data on the socket's IO buffer into Nsocket's internal buffer used for buffered
	//  reads. The purpose of this is to prevent the remote end from blocking in the event that the
	//  socket's system data buffer is full. It also provides a way of quantifying how much data
	//  is available for reading from the socket as unlike the system buffer, the amount of data in
	//  the Nsocket internal buffer can be obtained ( with GetBufferedDataLength() ).
	//  Parameter:
	//      theEnableFlag: true = start/enable, false = stop/disable.

	unsigned long getBufferedDataLength();
	//  Returns the amount of unread data currently in the internal buffer.
	//  Unless AutoReadBuffering is currently set, this may not represent the total amount of data
	//  available to be read from the socket, as it does not include any data in the sockets own
	//  buffer.

private:

	NSOCKET_STATUS		m_status;
	int					m_socket;
	int					m_notifySignal;
	int					m_originalNotifyFlags;
	char*					m_socketReadBuffer;
	int					m_socketReadBufferSize;
	std::queue<char>	m_readBuffer;
	Nthread*				m_autoBufferThread;
	Nevent				m_autoBufferThreadStop;
	Nevent				m_threadDoneUpdate;
	Nmutex				m_readbufferOwner;
	Nmutex				m_socketReadbufferOwner;
	int					m_closePipe[2];
	bool					m_closePipeCreatedFlag;

	static void* autoBufferProc( void* theParam );

	static void signalHandler(  int         theSignal,
                                siginfo_t*	theSigInfo,
                                void*	    theContext	);

	void waitForRawSocketEvent( bool* theTimedOutFlag = NULL, const Ntime theTimeout = 0 );

	void rawRead(   void*               theBuffer,
					const unsigned long	theLength,
					unsigned long*      theReadCount = NULL,
					const bool          theJustReadAvailableFlag = false,
					const Ntime&        theTimeout = 0,
					bool*               theTimedOutFlag = NULL          );

	bool socketDataAvailable();
	void createSocketReadBuffer();
	bool createClosePipe();
	void updateReadBuffer(	const bool theWaitForDataFlag = false,
                            const Ntime& theTimeout = 0,
                            bool* theTimedOutFlag = NULL            );
};


#endif
