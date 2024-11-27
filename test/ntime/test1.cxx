#include <iostream>

#include "nerror.h"
#include "ntime.h"

using namespace std;

int main( int ac, char*av[] )
{
    HANDLE_NERRORS;

    cout << Ntime::getCurrentLocalTime().getAsString() << endl;
    cout << "Uptime(seconds): " << Ntime::getUptime().getAsSeconds() << endl;
    cout << "System started on:" << ( Ntime::getCurrentLocalTime() - Ntime::getUptime() ).getAsString() << std::endl;

    return EXIT_SUCCESS;
}

