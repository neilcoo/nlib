#ifndef NTHREADPOOL_H
#define NTHREADPOOL_H

// NthreadPool v1.5 by Neil Cooper 18th November 2019
// Implements a generic thread pool object.
// Thread pools allow reuse of existing threads. In environments where multiple small work packages
// such as transactions need to be performed, this approach provides better performance than
// dynamically creating/destroying threads for each work package.

#include <vector>
#include <string>

#include "nmutex.h"
#include "nevent.h"
#include "nthread.h"


class NthreadPool
{
public:
    static const Nthread::CORE_AFFINITY DEFAULT_AFFINITY; // Means "use current default".

    typedef void ( *THREAD_PROC )( void* );
    // Type of user-supplied function to be run as thread.

    NthreadPool(    const size_t                 thePoolSize = 0,
                    const std::string            threadNameRoot = "",
                    const Nthread::CORE_AFFINITY defaultAffinity = Nthread::CORE_AFFINITY_ALL );
    // Constructor.
    // thePoolSize constrains the pool to have no more than thePoolSize threads.
    // If 0, the pool will grow as needed (ad infinitum) and SubmitJob() will never block.
    // defaultAffinity is used for SubmitJob() calls that do not explicitly provide affinity. 0 = all.


    virtual ~NthreadPool();
    // Note: Destructor will wait for pool to be idle (i.e. user tasks to complete) before it completes.

    void submitJob( THREAD_PROC                  theThreadProc,
                    void*                        theThreadParam = NULL,
                    const Nthread::CORE_AFFINITY affinity = NthreadPool::DEFAULT_AFFINITY );

    // Submit a job to be run by a pool thread.
    // theThreadProc is a user-supplied function to be run, that will have theThreadParam passed into
    // it as a parameter. This call may block until a pool thread is available, unless the
    // thread Pool was constructed with a pool size of 0 (meaning dynamic sizing).

    size_t getPoolSize();
    // Returns the total no. of threads in the pool (both active and not).
    // Always returns the same value unless the pool was created as dynamically sizing.

    void updatePoolAffinity( const Nthread::CORE_AFFINITY affinity = Nthread::CORE_AFFINITY_ALL );
    // Change the core affinity of all threads in the pool. Will switch threads currently running jobs too.
    // Also updates what the pool knows as DEFAULT_AFFINITY. 0 = All cores.

    void waitForIdle();
    // Function will return only when all pool threads are idle (i.e. no submitted jobs are still active).

private:
    typedef struct
        {
        Nthread*                thread;
        Nthread:: CORE_AFFINITY affinity;
        THREAD_PROC             userProc;
        void*                   userParams;
        Nevent                  startThread;
        Nevent*                 idleCountEvent;
        bool                    idle;
        bool*                   closing;

        } THREAD_CONTEXT;

    static void* threadProc( void* theThreadContext );
    THREAD_CONTEXT* extendPool( const Nthread::CORE_AFFINITY affinity );
    THREAD_CONTEXT* getIdleThread( const Nthread::CORE_AFFINITY affinity );

    bool                             m_closing;
    const size_t                     m_poolMaxSize;
    std::vector< THREAD_CONTEXT* >   m_pool;
    Nmutex                           m_poolOwner;
    Nevent                           m_idleCountEvent;
    Nthread:: CORE_AFFINITY          m_defaultAffinity;
    std::string                      m_threadNameRoot;
};

#endif
