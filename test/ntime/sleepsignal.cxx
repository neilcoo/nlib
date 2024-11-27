#include <iostream>
//#include <fstream>
//#include <string>
#include "nerror.h"
#include "ntime.h"
#include "nsocket.h"
#include "nevent.h"

using namespace std;
/*
// ------------------------------------------------------------------------------
bool LTrim( string& theString, const char theChar = ' ' )
{
  unsigned long offset = theString.find_first_not_of( theChar );
  unsigned long length = theString.length();

	if ( offset == string::npos )	// whole string is trim characters or empty
		theString.erase();
	else
		if (offset)
			theString.erase(0, offset);

	return( (offset != 0) && length );
}


// ------------------------------------------------------------------------------
bool RTrim( string& theString, const char theChar = ' ' )
{
	unsigned long offset = theString.find_last_not_of( theChar );
  unsigned long length = theString.length();

	if ( offset == string::npos )	// whole string is trim characters or empty
		theString.erase();
	else
    {
    offset++;
		if (offset != length )
			theString.erase(offset);
  }
	return( ( offset != length ) && ( length > 0 ) );
}
*/


int main( int ac, char* av[] )
{
    HANDLE_NERRORS;

    Nsocket sock;
    Nevent event;

    sock.listen( 5000, "localhost" );
    sock.notifyReady( &event );

    while ( true )
        {
        Ntime t =  Ntime::getCurrentLocalTime();
        Ntime tr;
        cout << "sleeping" << endl;

        bool status = Ntime::sleep( 5000, &tr );
        unsigned long e = t.getElapsed().getAsMs();
        unsigned long left = tr.getAsMs();
        if (status)
            cout << "slept OK. ";
        else
            cout << "interrupted. ";
        cout << left << " to go. " << e << " elapsed. total: " << left + e << endl;
        }
    return EXIT_SUCCESS;
}


