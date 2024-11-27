#ifndef NTHREAD_H
#define NTHREAD_H

// nthread v1.3 by Neil Cooper 18th Nov 2019
// Implements an easy-to-use pthread-based thread object.

#include <pthread.h>

#include <string>

#include "nbinary.h"
#include "nevent.h"

class Nthread
{
public:
    typedef Nbinary CORE_AFFINITY;

    static const CORE_AFFINITY CORE_AFFINITY_ALL;

    typedef void* ( *NTHREAD_THREAD_PROC )( void* );
    // type of function that will run as a thread

    typedef enum
        {
        // Time-sharing scheduling models
        DEFAULT,       // Standard Linux scheduler
        BATCH,         // Scheuduler assumes task is CPU-intensive
        IDLE,          // Runs jobs at very low priority (even lower than nice +19)
        // Realtime scheduling models
        FIFO,          // First in-first out scheduling
        ROUND_ROBIN,   // Round-robin scheduling
        DEADLINE       // not fully supported by pthreads API (yet?)
        } SCHEDULING_MODEL;


    Nthread( NTHREAD_THREAD_PROC  theProcess,
    void*                theParameter = NULL,
    const std::string    theName = "",
    const bool           theCreateDetachedFlag = false );
    // constructor
    // parameters:
    // theProcess:  pointer to function that will run as a thread
    // theParameter: parameter that will be passed to thread function.
    // theName: Unique name for this thread. Empty = use parent program name.
    // theCreateDetachedFlag: if true, creates the thread as 'detached'

    virtual ~Nthread();

    pid_t getTid(); // Get TID of the thread. Blocks until thread is created.

    void setNice( int priority ); // -20 highest priority, 19 lowest priority

    int getNice();

    void setThreadAffinity( const CORE_AFFINITY enabledCores = CORE_AFFINITY_ALL );
    // Sets which core(s) the thread will run on. 0 = all.
    // Parameter is a bitmask, LSB = core0, MSB = highest in the current platform.
    // 0 = do not use associated core, 1 = use associated core.

    CORE_AFFINITY getThreadAffinity();
    // Returns currently set affinity.

    void setSchedulingModel(    const SCHEDULING_MODEL theModel = SCHEDULING_MODEL::DEFAULT,
                                const int schedulingPriority = 0 );
    // Note: Scheduling priority is ignored for non-realtime scheduling
    // models (DEFAULT, BATCH and IDLE models).

    SCHEDULING_MODEL getSchedulingModel();

    void setSchedulingPriority( const int thePriority = 0 );
    // Note: Scheduling priority is ignored and must be 0 for non-realtime scheduling
    // models (DEFAULT, BATCH and IDLE models). You're probably looking for setNice().

    int getSchedulingPriority();

    void getReturnValue( void** theReturnParam = NULL );
    // Waits for thread to exit. If the storage provided is not NULL,
    // it is filled with the value returned from the thread.
    // NB: Only works for threads not created as detached.
    // theReturnParam: address of storage to hold the thread return value

private:

    typedef struct
        {
        NTHREAD_THREAD_PROC  threadProc;
        void*                threadParams;
        pid_t*               tid;
        Nevent*              tidGot;
        } CallerParams;

    static void* getTidAndCall( void* theParams );

    void killThread();

    CallerParams m_callerParams;
    pid_t        m_tid;
    pthread_t    m_threadId;
    bool         m_created;
    bool         m_detached;
    bool         m_joined;
    Nevent       m_tidGot;
};

#endif

