//
//  Hello World client in C++
//  Connects REQ socket to tcp://localhost:5555
//  Sends "Hello" to server, expects "World" back
//

#include <string>
#include <sstream>
#include <iostream>

#include "nerror.h"
#include "nzmq.h"

using namespace std;

int main ()
{
    HANDLE_NERRORS;

    Nzmq mq( "tcp://localhost:5555", Nzmq::MODE::REQ );

    char buf[ 1024 ];

    //  Do 10 requests, waiting each time for a response
    for (int i = 0; i < 10; i++)
        {
        ostringstream msgStream;
        msgStream << "Hello " << i;
        string msg = msgStream.str();
        cout << "Sending '" << msg << "'" << endl;
        size_t len = mq.tx( msg );
        cout << "sent " << len << " bytes" << endl;
        len = mq.rx( msg );
        cout << "Received " << len << " bytes: '" << msg << "'" << endl;
        }

    return EXIT_SUCCESS;
}
