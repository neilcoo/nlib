#include <iostream>
#include "nerror.h"
#include "nthreadPool.h"
#include "ntime.h"

Nmutex textout;

using namespace std;

typedef struct
{
   int index;
   int msDelay;
} THREAD_PARAMS;

void Threadproc(void* theParam)
{
    THREAD_PARAMS* p = (THREAD_PARAMS*)theParam;
    // if (p->index == 3 )
        // ERROR("Fake error from thread 3");
    textout.lock();
    cout << p->index << ": Delaying for " << p->msDelay << " ms" << endl;
    textout.unlock();
    Ntime::sleep(p->msDelay);
    textout.lock();
    cout << p->index << ": Ending" << endl;
    textout.unlock();
    //   while(1);
}


int main(int ac, char* av[] )
{
    HANDLE_NERRORS;

    const int NUMBER_OF_THREADS = 5;

    NthreadPool* pool = new NthreadPool( NUMBER_OF_THREADS );
    THREAD_PARAMS threadParams[  NUMBER_OF_THREADS ];

    for ( int i = 0; i < NUMBER_OF_THREADS; i++ )
        {
        threadParams[ i ].index = i;
        float rndf = rand();
        rndf = rndf / RAND_MAX;
        rndf = rndf * 1000;
        threadParams[ i ].msDelay = rndf;
        textout.lock();
        cout << "Submitting " << i << endl;
        textout.unlock();
        pool->submitJob( Threadproc, (void*)&threadParams[ i ] );
        }
/*
    textout.lock();
    cout << "main entering WaitForIdle" << endl;
    textout.unlock();
    pool->WaitForIdle();
    textout.lock();
    cout << "entering WaitForIdle2" << endl;
    textout.unlock();
    pool->WaitForIdle();
*/
    Ntime::sleep(100);
    textout.lock();
    cout << "main deleting pool" << endl;
    textout.unlock();
    delete pool;
    cout << "main done" << endl;
}

