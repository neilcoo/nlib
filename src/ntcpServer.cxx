// ntcpServer.cxx by Neil Cooper. See ntcpServer.h for documentation
#include "ntcpServer.h"

#include "nerror.h"

using namespace std;


NtcpServer::NtcpServer( NTCPSERVER_THREAD_PROC  theClientProcess,
                        const unsigned short    thePort,
                        const char*             theIpAddress,
                        void*                   theUserParam )
{
   start( theClientProcess, thePort, theIpAddress, theUserParam );
}


NtcpServer::NtcpServer()
{
}


NtcpServer::~NtcpServer()
{
  stop();
}


void NtcpServer::safeGarbageCollector( void* theParam )
{
	NtcpServer& us = *(NtcpServer*)theParam;

	while(  us.m_clientList.size() || ( us.m_serverSocket.getStatus() != Nsocket::CLOSED ) )
		{
		us.m_collectGarbage.wait();
		us.m_clientListOwner.lock();

		unsigned long i = 0;
		while ( i < us.m_clientList.size() )
			{
			if ( !us.m_clientList[i]->imDoneFlag )
				i++;
			else
				{
				// Wait for thread to terminate
            us.m_clientList[i]->clientThread->getReturnValue();
				delete us.m_clientList[i]->clientThread;
				delete us.m_clientList[i];
				us.m_clientList.erase( us.m_clientList.begin() + i );
				}
			}
		us.m_clientListOwner.unlock();
		}
}


void* NtcpServer::garbageCollector( void* theParam )
{
	NERROR_HANDLER( safeGarbageCollector( theParam ) );
	return NULL;
}


void NtcpServer::safeClientThread( void* theParam )
{
	THREAD_CONTEXT& context = *(THREAD_CONTEXT*)theParam;
	NtcpServer& us = *(context.us);

	// Call the user-process
	us.m_clientProcess( context.params );

	// Clean up after user process has returned
	if ( context.params.clientSocket.getStatus() != Nsocket::CLOSED )
		context.params.clientSocket.closeSocket();

	context.imDoneFlag = true;
	us.m_collectGarbage.signal();
}


void* NtcpServer::clientThread( void* theParam )
{
	NERROR_HANDLER( safeClientThread( theParam ) );
	return NULL;
}


void NtcpServer::start( NTCPSERVER_THREAD_PROC  theClientProcess,
                        const unsigned short    thePort,
                        const char*             theIpAddress,
                        void*                   theUserParam      )
{
	m_serverSocket.listen( thePort, theIpAddress );
  	m_clientProcess = theClientProcess;
	m_garbageCollector = new Nthread( garbageCollector, this );

	do
		{
		m_serverSocket.waitForSocketEvent();
		if ( m_serverSocket.acceptIsAvailable() )
			{
			THREAD_CONTEXT* context = new THREAD_CONTEXT;
			context->imDoneFlag = false;
			context->params.userParam = theUserParam;
			context->us = this; // used by static methods for member access

			m_serverSocketOwner.lock();
			// Accept, but allow for an accept failure because the socket got closed (i.e. Stop() called ).
			try
				{
				m_serverSocket.accept( context->params.clientSocket );
				}
			catch( NerrorException& e )
				{
				if ( m_serverSocket.getStatus() != Nsocket::CLOSED )
					{
					m_serverSocketOwner.unlock();
					throw( e );
					}
				}
			m_serverSocketOwner.unlock();

			m_clientListOwner.lock();
			// Create the thread inside the lock so that it can't delete itself from
			// the client list before it has been added because the list is locked.
			context->clientThread = new Nthread( clientThread, context  );
			m_clientList.push_back( context );
			m_clientListOwner.unlock();
			}
		}
		while ( m_serverSocket.getStatus() == Nsocket::LISTENING );

	// We should only get here after Stop has been called.
	// Wait for garbage collector thread to exit
   m_garbageCollector->getReturnValue();
	delete m_garbageCollector;
}


bool NtcpServer::stop()
{
	// We'll just use closing of the server port to communicate our intentions to the server thread
	bool status = ( m_serverSocket.getStatus() ==  Nsocket::LISTENING );

	if ( status )
		{
		m_serverSocketOwner.lock();
		m_serverSocket.closeSocket();
		m_serverSocketOwner.unlock();

		if ( m_clientList.size() == 0 )
			// Wake up the garbage collector to allow it to exit
			m_collectGarbage.signal();
		else
			{
			// close any/all client sockets as a way to signal client threads to terminate.
			m_clientListOwner.lock();
			for ( unsigned long i=0; i < m_clientList.size(); i++ )
				m_clientList[i]->params.clientSocket.closeSocket();
			m_clientListOwner.unlock();
			}
	}
	return status;
}
