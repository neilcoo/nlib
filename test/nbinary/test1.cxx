#include <iostream>
#include "nerror.h"
#include "nbinary.h"

using namespace std;


int main(int ac, char* av[] )
{
    HANDLE_NERRORS;

    Nbinary bin1( "7239234834" ); // some random number
    cout << "length test: Should be 33: " << bin1.length() << endl;
    cout << "hex test:    Should be 1AF7DF512: " << bin1.getAsHex() << endl;
    cout << "Binary test: Should be 110101111011111011111010100010010:" << endl << "                       " << bin1.getAsBinary() << endl;

Nbinary bin2 = bin1;
    cout << "length test: " << bin2.length() << endl;
    cout << "hex test: " << bin2.getAsHex() << endl;
    cout << "Binary test: '" << bin2.getAsBinary() << "'" << endl;

for ( int i = bin1.length() + 3; i> -1; i-- )
{
    cout << endl <<  "Setting bit length to " << i << ": " << ( bin2.setBitLength( i, true ) ? "OK" : "FAIL" ) << endl;
    cout << "length test: Should be " << i << ": " << bin2.length() << endl;
    cout << "hex test: " << bin2.getAsHex() << endl;
    cout << "Binary test: " << bin2.getAsBinary() << endl;
}

}

