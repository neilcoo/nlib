#include <iostream>
#include <string.h>

#include "nsocket.h"
#include "nerror.h"

using namespace std;

Nsocket sock;

void Log(const char* theString)
{
	cout << theString << endl;
}


void reportStatus( Nsocket& thesocket )
{
	switch( thesocket.getStatus() )
		{
		case Nsocket::CLOSED:
			cout << "CLOSED";
			break;

		case Nsocket::LISTENING:
			cout << "LISTENING";
			break;

		case Nsocket::CONNECTED:
			cout << "CONNECTED";
			break;

		case Nsocket::REMOTE_CLOSED:
			cout << "REMOTE CLOSED";
			break;

		default:
			cout << "UNKNOWN";
			break;
		}
	cout << endl;
}


void acceptTest()
{
    cout << "listener status1:";
	reportStatus( sock );

	Log("going to listen on 4567");
    sock.listen( 4567 );
	cout << "status:";
	reportStatus( sock );

    if ( sock.getStatus() == Nsocket::LISTENING )
        do
            {
            Log("Going to accept");
            Nsocket client;
            sock.accept( client );

            bool end = false;
            while (client.getStatus() != Nsocket::REMOTE_CLOSED)
                {
                char buf[1024];

                unsigned long gotLength;


                gotLength = client.read(  &buf, 4, true );
                cout << "  gotLength:" << gotLength << endl;

                if ( gotLength )
                    {
                    buf[gotLength] = 0;
                    cout << "Got: '"<< buf << "'" << endl;
                    }
                cout << "socket status:";
                reportStatus( client );
                sleep( 1 );
                }
		    } while( true );
}


void ConnectTest()
{
    const char* msg = "hello ";
    do
        {
        sock.connectTo( 23, "172.30.8.210" );
        reportStatus( sock );
        if ( sock.getStatus() == Nsocket::CONNECTED )
            do
                {
                cout << "Write returned " << sock.write( msg, strlen( msg ) ) << endl;
                reportStatus( sock );
                }	while ( sock.getStatus() != Nsocket::REMOTE_CLOSED );
        } while( true );
}


int main( int ac, char* av[] )
{
    HANDLE_NERRORS;

	acceptTest();

//	ConnectTest();

	return 1;
}


