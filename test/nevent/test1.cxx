#include <iostream>
#include "nthread.h"
#include "ntime.h"
#include "nevent.h"

using namespace std;

struct PARAMS
{
   unsigned short threadId;
	Nevent* event;
};



void* sleepthreadproc(void* theParam)
{
    PARAMS& param = *(PARAMS*)theParam;

    cout << "sleepthread " << param.threadId << " started" << endl;
    Ntime::sleep( 5000 - ( param.threadId * 1000 ) );

    cout << "thread " << param.threadId << " signal" << endl;
    param.event->signal();

    return (void*)theParam;
}


void* waitthreadproc(void* theParam)
{
    PARAMS& param = *(PARAMS*)theParam;

    cout << "waitthread " << param.threadId << " started" << endl;

    if ( param.event->wait( 0 ) )
        cout << "waitthread " << param.threadId << " signalled" << endl;
    else
        cout << "waitthread " << param.threadId << "timedout" << endl;

    return (void*)theParam;
}


int main(int ac, char* av[] )
{
    cout << "Beginning event test" << endl;

    const int NUMBER_OF_THREADS = 3;

    PARAMS   params[ NUMBER_OF_THREADS ];
    Nthread* sleepthreads[ NUMBER_OF_THREADS ];
    Nthread* waitthreads[ NUMBER_OF_THREADS ];

    Nevent event[ NUMBER_OF_THREADS ];

    bool status = true;

    for ( unsigned short i = 0; i < NUMBER_OF_THREADS; i++ )
        {
        params[i].threadId = i;
        params[i].event = &( event[i] );

        sleepthreads[i] = new Nthread(sleepthreadproc,  &params[i] );
        waitthreads[i] = new Nthread( waitthreadproc, &params[i] );
        }


    for ( unsigned short i = 0; i < NUMBER_OF_THREADS; i++ )
        {
        void* sleepreturnval;
        void* waitreturnval;
        sleepthreads[i]->getReturnValue( &sleepreturnval );
        waitthreads[i]->getReturnValue( &waitreturnval );
        cout << "getReturnValue() for " << i << " returned " << sleepreturnval << "," << waitreturnval << endl;
        cout << " Exit ID values returned are " << ((PARAMS*)sleepreturnval)->threadId << "," << ((PARAMS*)waitreturnval)->threadId << endl;
        }
}
