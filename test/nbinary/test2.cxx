#include <iostream>
#include "nerror.h"
#include "nbinary.h"

using namespace std;


int main(int ac, char* av[] )
{
    HANDLE_NERRORS;

    Nbinary bin( "7239234834" ); // some random number
    cout << "length test: Should be 33: " << bin.length() << endl;
    cout << "hex test:    Should be 1AF7DF512: " << bin.getAsHex() << endl;
    cout << "Binary test: Should be 110101111011111011111010100010010:" << endl << "                       " << bin.getAsBinary() << endl;
}
