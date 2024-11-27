#include <iostream>

#include "nerror.h"
#include "nthread.h"
#include "nmutex.h"
#include "ntime.h"
#include "nrandom.h"

using namespace std;

Nrandom randGen;

struct PARAMS
{
	unsigned short param1;
};


void* threadproc(void* theParam)
{
    PARAMS& param = *(PARAMS*)theParam;
    LOG( "threadparam: ", param.param1 );
    Ntime::sleep( randGen.getRandomNumber( 500, 2000 ) );
    LOG( "thread ", param.param1, " ending" );
    return (void*)(&param.param1);
}


int main(int ac, char* av[] )
{
    HANDLE_NERRORS;

	LOG( "Beginning thread test" );

	const int NUMBER_OF_THREADS = 1;

	PARAMS	params[ NUMBER_OF_THREADS ];
	Nthread* threads[ NUMBER_OF_THREADS ];

	for ( unsigned short i = 0; i < NUMBER_OF_THREADS; i++ )
		{
		params[i].param1 = i;
		threads[i] = new Nthread(threadproc,  &params[i] );
		}

    LOG("Threads created ok");

	for ( unsigned short i = 0; i < NUMBER_OF_THREADS; i++ )
		{
        LOG("Deleting ", i );
        delete threads[i];
		}

    LOG("Threads deleted ok");
/*
	for ( unsigned short i = 0; i < NUMBER_OF_THREADS; i++ )
		{
	   void* returnval;
      threads[i]->GetReturnValue( &returnval );
		Print( "Return value for " << i << " is " << *(unsigned short*)returnval );
		}
*/
}
