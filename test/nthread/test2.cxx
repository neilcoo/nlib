#include <iostream>
#include "nthread.h"
#include "nmutex.h"
#include "ntime.h"
#include "nrandom.h"

using namespace std;

Nrandom randGen;

Nmutex printGuard;

#define Print( msg... )    \
{                          \
   printGuard.lock();      \
   cout << msg << endl;    \
   printGuard.unlock();    \
}

struct PARAMS
{
	unsigned short param1;
};


void* threadproc(void* theParam)
{
    PARAMS& param = *(PARAMS*)theParam;
    Print( "threadparam: " << param.param1 );
    Ntime::sleep( randGen.getRandomNumber( 500, 2000 ) );
    Print( "thread " << param.param1 <<" ending" );
    return (void*)(&param.param1);
}


int main(int ac, char* av[] )
{
    Print( "Beginning thread test" );

    const int NUMBER_OF_THREADS = 3;

    PARAMS	params[ NUMBER_OF_THREADS ];
    Nthread* threads[ NUMBER_OF_THREADS ];

    for ( unsigned short i = 0; i < NUMBER_OF_THREADS; i++ )
        {
        params[i].param1 = i;
        threads[i] = new Nthread(threadproc, &params[i] );
        }

    for ( unsigned short i = 0; i < NUMBER_OF_THREADS; i++ )
        {
        void* returnval;
        threads[i]->getReturnValue( &returnval );
        Print( "Return value for " << i << " is " << *(unsigned short*)returnval );
        }
}
