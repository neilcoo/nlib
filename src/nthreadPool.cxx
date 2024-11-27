// nthreadPool.cxx by Neil Cooper. See nthreadPool.h for documentation
#include "nthreadPool.h"

#include "nerror.h"
#include "nmutex.h"

const Nthread::CORE_AFFINITY NthreadPool::DEFAULT_AFFINITY(-1);
// DEFAULT_AFFINITY is a magic number not an actual affinity.
// It is used with SubmitJob() to mean use the default (set by calling UpdatePoolAffinity() ).
// If UpdatePoolAffinity has never been called, it is equivalent to Nthread::CORE_AFFINITY_ALL.

NthreadPool::NthreadPool( const size_t                 thePoolSize,
                          const std::string            threadNameRoot,
                          const Nthread::CORE_AFFINITY defaultAffinity ) : m_closing( false ),
                                                                           m_poolMaxSize( thePoolSize ),
                                                                           m_idleCountEvent( true ),
                                                                           m_defaultAffinity( defaultAffinity ),
                                                                           m_threadNameRoot( threadNameRoot )
{
    // 0 explicitly means all cores. Default affinity starts out as all cores.
    if ( m_defaultAffinity == DEFAULT_AFFINITY )
        m_defaultAffinity = Nthread::CORE_AFFINITY_ALL;

    // Initialise worker threads.
    // Doing this rather than on demand front-loads the performance hit, and also avoids
    // a corner case problem with calling WaitForIdle() before SubmitJob() has ever been called,
    // so m_pool.size() would be zero.
    for ( size_t i = 0; i < thePoolSize; i++ )
        {
        THREAD_CONTEXT* idleThread = extendPool( m_defaultAffinity );
        if ( idleThread )
            {
            idleThread->idle = true;   // ExtendPool() Also atomically marks the thread as not
            m_idleCountEvent.signal(); // idle. Mark it as idle and increment the idle thread count.
            }
        }
}


NthreadPool::~NthreadPool()
{
    m_closing = true;
    waitForIdle(); // wait for all user jobs to end

    size_t poolSize = m_pool.size();

    // Don't leave any thread blocked on event otherwise we can't delete it.
    // Since m_closing is true, they will skip starting client jobs.
    for ( size_t i = 0; i < poolSize; i++ )
        m_pool[i]->startThread.signal();

    for ( size_t i = 0; i < poolSize; i++ )
        {
        THREAD_CONTEXT* context = m_pool[i];
        context->thread->getReturnValue();
        delete context->thread;
        delete context;
        }
}


void* NthreadPool::threadProc( void* theThreadContext )
{
    THREAD_CONTEXT* context = (THREAD_CONTEXT*)theThreadContext;

    while( ! *( context->closing ) )
        {
        context->startThread.wait();

        if ( ! *( context->closing ) )
            context->userProc( context->userParams );

        context->idle = true;
        context->idleCountEvent->signal();
        }

    return NULL; // Nthread takes a void* (*)(void*) type, but we don't allow worker threads to return values
}


NthreadPool::THREAD_CONTEXT* NthreadPool::extendPool( const Nthread::CORE_AFFINITY theAffinity )
{
    // Add a thread to the pool. Assumes thread is going to be used so returns it already
    // marked as not idle, so that it can't be found and used by another thread doing a SubmitJob.

    THREAD_CONTEXT* context = new THREAD_CONTEXT;
    context->idle = false; // Don't allow a preempting thread to also find this one
    context->idleCountEvent = &m_idleCountEvent;
    context->closing = &m_closing;
    std::ostringstream threadName;

    if (  m_threadNameRoot.size() > 0 )
        threadName << m_threadNameRoot << m_pool.size();

    context->thread = new Nthread( NthreadPool::threadProc, (void*)context, threadName.str() );

    if ( theAffinity.getAsInt() != Nthread::CORE_AFFINITY_ALL.getAsInt() )
        context->thread->setThreadAffinity( theAffinity );

    m_pool.push_back( context );

    return context;
}


NthreadPool::THREAD_CONTEXT* NthreadPool::getIdleThread( const Nthread::CORE_AFFINITY theAffinity ) // Returns NULL if we cant grow and no idle thread found
{
    THREAD_CONTEXT* idleThread = NULL;

    m_poolOwner.lock();
    // find out if theres a idle thread
    if ( !m_idleCountEvent.currentState() )
    // No idle thread. If pool size is unbounded or smaller than max we can make one
        if ( !m_poolMaxSize || ( m_pool.size() < m_poolMaxSize ) )
            idleThread = extendPool( theAffinity );

    if ( !idleThread )  // Wait for thread to become idle
        {
        size_t poolSize = m_pool.size();

        m_idleCountEvent.wait();

        for ( size_t i = 0; ( i < poolSize ) && ( !idleThread ); i++ )
            if ( m_pool[i]->idle )
                {
                idleThread = m_pool[i];
                idleThread->idle = false;  // Don't allow a preempting thread to also find this one
                if ( idleThread->affinity.getAsInt() != theAffinity.getAsInt() )
                    {
                    idleThread->thread->setThreadAffinity( theAffinity );
                    idleThread->affinity = theAffinity;
                    }
                }
        }
    m_poolOwner.unlock();
    return idleThread;
}


void NthreadPool::submitJob( THREAD_PROC                  theThreadProc,
                             void*                        theThreadParam,
                             const Nthread::CORE_AFFINITY affinity )
{
    Nthread::CORE_AFFINITY useAffinity = affinity;
    if ( useAffinity == DEFAULT_AFFINITY )
        useAffinity = m_defaultAffinity;

    if ( m_closing )
        ERROR( "NthreadPool: SubmitJob called on NthreadPool object being destructed." );
    else
        {
        THREAD_CONTEXT* idleThread = getIdleThread( useAffinity );

        if ( !idleThread )
            ERROR( "NthreadPool: No idle thread to submit job to." );

        idleThread->userProc = theThreadProc;
        idleThread->userParams = theThreadParam;
        idleThread->startThread.signal();
        }
}


size_t NthreadPool::getPoolSize()
{
    return m_pool.size();
}


void NthreadPool::updatePoolAffinity( const Nthread::CORE_AFFINITY theAffinity )
{
    m_defaultAffinity = theAffinity;

    for ( size_t i = 0; i < m_pool.size(); i++ )
        if ( m_pool[i]->affinity.getAsInt() != m_defaultAffinity.getAsInt() )
            {
            m_pool[i]->thread->setThreadAffinity( m_defaultAffinity );
            m_pool[i]->affinity = m_defaultAffinity;
            }
}


void NthreadPool::waitForIdle() // Wait for all work threads to finish and be idle
{
    m_poolOwner.lock();

    for ( size_t i = 0; i < m_pool.size(); i++ )
        {
        m_idleCountEvent.wait();
        m_idleCountEvent.signal(); // Don't consune the event
        }
    m_poolOwner.unlock();
}

