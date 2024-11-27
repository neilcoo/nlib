#include <iostream>
#include <iomanip>
#include "nerror.h"

#include "nrandom.h"

using namespace std;


int main(int ac, char* av[])
{
   HANDLE_NERRORS;

   Nrandom rand;

    cout << "RAND_MAX is " << RAND_MAX << endl;

    float max = 0;
    float min = 99999;
    while ( true )
        {
        float x = rand.getRandomNumber(1,100 );
        if( x > max )
            {
            max = x;
            cout << setprecision(99) << min << "   " << max << endl;
            }
        if( x < min )
            {
            min = x;
            cout << setprecision(99) << min << "   " << max << endl;
            }
        }

   return EXIT_SUCCESS;
}
