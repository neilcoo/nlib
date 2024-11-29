#ifndef NMUTEX_H
#define NMUTEX_H

// nmutex v1.1 by Neil Cooper 14th Nov 2014
// Implements an easy-to-use pthread-based mutex object.

#include <string>        // for std::string
#include <pthread.h>    // for pthread_mutex_t and pthread_cond_t

class Nmutex
{
public:
    //  The mutex type determines what happens if a thread attempts to lock
    //  a mutex it already owns by calling Lock().
    //  FAST simply suspends the calling thread forever.
    //  ERRORCHECK returns immediately with an error code.
    //  RECURSIVE returns immediately with a success code.
    //  The number of times the thread owning the mutex has locked it is recorded in the mutex.
    //  The owning thread must call Release the same number of times before
    //  the mutex returns to the unlocked state.
    //  RECURSIVE best approximates WIN32 behaviour


#if defined ( __CYGWIN__ ) || defined( __ANDROID__ ) 
    typedef enum
        {
        FAST            = PTHREAD_MUTEX_NORMAL,
        ERRORCHECK	= PTHREAD_MUTEX_ERRORCHECK,
        RECURSIVE	= PTHREAD_MUTEX_RECURSIVE

        }	MUTEX_TYPE;
#else
    typedef enum
        {
        FAST            = PTHREAD_MUTEX_FAST_NP,
        ERRORCHECK	= PTHREAD_MUTEX_ERRORCHECK_NP,
        RECURSIVE	= PTHREAD_MUTEX_RECURSIVE_NP

        }	MUTEX_TYPE;
#endif


    Nmutex( MUTEX_TYPE theType = FAST );
    virtual ~Nmutex();

    void lock();
    // Lock the mutex. if the mutex is currently not locked, it becomes
    // locked and owned by the calling thread and returns immediately.
    // If it is already locked by another thread, the behaviour is
    // dependant on the type of mutex as decribed above.


    bool tryLock();
    // Behaves as lock() except does not block the calling thread if the
    // mutex is already locked by another thread.
    // Return:
    //    true = success, false = mutex is already locked

    void unlock();
    // Unlocks the mutex (which has been previously locked by the calling thread).

    private:
    pthread_mutex_t	m_mutex;
};

#endif



