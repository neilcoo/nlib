//
//  Hello World server in C++
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
//

#include <string>
#include <sstream>
#include <iostream>

#include "nerror.h"
#include "nzmq.h"

using namespace std;

int main()
{
    HANDLE_NERRORS;

    Nzmq mq( "tcp://*:5555", Nzmq::MODE::REP, Nzmq::TYPE::BIND );
    int counter = 0;

    while( true )
        {
        string msg;
        size_t len = mq.rx(msg);
        cout << "Received " << len << " bytes: '" << msg << "'" << endl;
        ostringstream msgStream;
        msgStream << "World " << counter++;
        msg = msgStream.str();
        cout << "Sending '" << msg << "'" << endl;
        len = mq.tx(msg);
        cout << "sent " << len << " bytes" << endl;
        }

    return EXIT_SUCCESS;
}
