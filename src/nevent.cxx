// nevent.cxx by Neil Cooper. See nevent.h for documentation
#include "nevent.h"

#include <errno.h>   // for ETIMEDOUT
#include <pthread.h>

#include "nerror.h"

using namespace std;

Nevent::Nevent( const bool           theCountingEventFlag,
                const unsigned long  theInitialState,
                const bool           theManualResetFlag   ) :
                    m_isCountingEvent( theCountingEventFlag ),
                    m_isManualReset( theManualResetFlag         )
{
    if  ( m_isCountingEvent )
        m_eventUnion.m_countingEvent = theInitialState;
    else
        m_eventUnion.m_boolEvent = ( theInitialState > 0 );

    // Set up the attributes of our mutex
    pthread_mutexattr_t attributes;
    if ( pthread_mutexattr_init( &attributes ) )
        ERROR( "Nevent: Can't create pthread mutex atributes" );

    // The mutex kind determines what happens if a thread attempts to lock
    // a mutex it already owns by calling pthread_mutex_lock().
    // PTHREAD_MUTEX_FAST_NP simply suspends the calling thread forever.
    // PTHREAD_MUTEX_ERRORCHECK_NP  returns immediately with the error code EDEADLK.
    // PTHREAD_MUTEX_RECURSIVE_NP returns immediately with a success return code.
    // The number of times the thread owning the mutex has locked it is recorded in the mutex.
    // The owning thread must call pthread_mutex_unlock the same number of times before
    // the mutex returns to the unlocked state.
    // PTHREAD_MUTEX_RECURSIVE_NP best approximates WIN32 behaviour

#ifdef __CYGWIN__
    if ( pthread_mutexattr_settype( &attributes, PTHREAD_MUTEX_NORMAL ) ) // Still need to confirm this is really functionally identical to PTHREAD_MUTEX_FAST_NP
#else
    if ( pthread_mutexattr_settype( &attributes, PTHREAD_MUTEX_FAST_NP ) )
#endif
        ERROR( "Nevent: Can't set pthread mutex type attribute" );

    if ( pthread_mutex_init( &m_accessMutex, &attributes ) )
        ERROR( "Nevent: Can't initialise pthread mutex attributes" );

    // pthread_mutexattr_destroy currently does nothing in Linux Pthreads
    // but we'll do it anyway here for future-proofing and portability.
    if ( pthread_mutexattr_destroy( &attributes ) )
        ERROR( "Nevent: Can't destroy pthread mutex attributes" );

    if ( pthread_cond_init( &m_condition, NULL ) )
        ERROR( "Nevent: Can't initialise pthread condition" );
}


Nevent::~Nevent()
{
    // Destroying a pthread condition that someone is waiting on returns EBUSY
    // but we're already in the destructor.
    int status = pthread_cond_destroy( &m_condition );
    if ( status )
        NWARN( status, "Nevent::~Nevent: Can't destroy pthread condition" );

    status = pthread_mutex_destroy( &m_accessMutex ); // Not currently necessary in Linux

    if ( status )
        NWARN( status, "Nevent::~Nevent: Can't destroy pthread mutex" );
}


void Nevent::signal()
// If bool event: Set the event to the signalled state.
// If counting event: Increment the event towards the signalled state.
{
    if ( m_isCountingEvent )
        changeCountingEventState( 1 );
    else
        changeBoolEventState( true );
}


void Nevent::unsignal()
// Only needed for manual reset events.
// If bool event: set the event to the unsignalled state.
// If counting event: decrement event towards unsignalled state.
{
    if ( m_isCountingEvent )
        changeCountingEventState( -1 );
    else
        changeBoolEventState( false );
}


void Nevent::reset()
{
    if ( m_isCountingEvent )
        resetCountingEventState();
    else
        changeBoolEventState( false );
}


unsigned long Nevent::currentState()
// Returns current state of event.
// No guarantees about whether it is still accurate by the time the function returns.
{
    unsigned long state = 0;
    if ( m_isCountingEvent )
        state = m_eventUnion.m_countingEvent;
    else
        state = m_eventUnion.m_boolEvent ? 1 : 0;

    return state;
}


bool Nevent::wait( const Ntime theTimeout )
{
    struct timespec endTime;
    bool mutexLocked = false;
    bool timedOut = false;
    bool eventClear = false;
    bool timedWait =  !theTimeout.isZeroTime();

    if ( timedWait )
        {
        Ntime timeSum = Ntime::getCurrentLocalTime();
        timeSum += theTimeout;
        endTime = timeSum.getAsTimespec();
        }

    int lockStatus = pthread_mutex_lock( &m_accessMutex );

    if ( 0 == lockStatus )
        mutexLocked = true;
    else
        NERROR( lockStatus, "Nevent::Wait: Error locking access mutex." );

    // Wait until the event is signalled, we time out or a mutex error occurs
    // (mutex is unlocked implicitly while we wait)

    eventClear = m_isCountingEvent ?
                            ( 0 == m_eventUnion.m_countingEvent ) :
                            ( !m_eventUnion.m_boolEvent );

    while( eventClear && ( !timedOut ) )
        {
        if ( timedWait )
            {
            int retVal = pthread_cond_timedwait( &m_condition, &m_accessMutex, &endTime );
            if ( retVal )
                {
                if  ( retVal == ETIMEDOUT )
                    timedOut = true;
                else
                    NERROR( retVal, "Nevent::Wait: pthread_cond_timedwait failed." );
                }
            }
        else
            {
            int retVal = pthread_cond_wait( &m_condition, &m_accessMutex );
            if ( retVal )
                NERROR( retVal, "Nevent::Wait: pthread_cond_wait failed." );
            }

        eventClear = m_isCountingEvent ?
                            ( 0 == m_eventUnion.m_countingEvent ) :
                            ( !m_eventUnion.m_boolEvent );
        }

    // If wait was successful, reset the event unless manual mode
    if( !timedOut )
        {
        if ( m_isManualReset )
            pthread_cond_signal( &m_condition );  // Explicitly let anyone else through
        else
            {
            if ( m_isCountingEvent )
                --m_eventUnion.m_countingEvent;
            else
                m_eventUnion.m_boolEvent = false; // Just this being false when we unlock will let anyone else through due to eventClear test above
            }
        }

    if( mutexLocked )
        {
        lockStatus = pthread_mutex_unlock( &m_accessMutex );
        if ( lockStatus )
            NERROR( lockStatus, "Nevent::Wait: Error unlocking access mutex." );
        }

    return ( !timedOut );
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------

void Nevent::changeBoolEventState( const bool   theState )
{
    int signalVal = 0;
    int retVal = pthread_mutex_lock( &m_accessMutex );

    if ( retVal )
        NERROR( retVal, "Nevent::ChangeBoolEventState: Error locking access mutex." );

    m_eventUnion.m_boolEvent = theState;

    // Only want to signal if the event is set to signalled
    if( theState )
        signalVal = pthread_cond_signal( &m_condition );

    retVal = pthread_mutex_unlock( &m_accessMutex );
    if ( retVal )
        NERROR( retVal, "Nevent::ChangeBoolEventState: Error unlocking access mutex." );

    // Delay throwing of exception until after we unlocked mutex just so we dont leave a locked mutex around
    if ( signalVal )
        NERROR( signalVal, "Nevent::ChangeBoolEventState: Can't signal condition." );
}



void Nevent::changeCountingEventState( const long  theChange )
{
    int signalVal = 0;
    int retVal = pthread_mutex_lock( &m_accessMutex );

    if ( retVal )
        NERROR( retVal, "Nevent::ChangeCountingEventState: Error locking access mutex." );

    if ( ( theChange > 0 ) || ( m_eventUnion.m_countingEvent >= labs( theChange ) ) ) // Ensure change wont leave counter < 0
        {
        m_eventUnion.m_countingEvent += theChange;

        // Only want to signal the condition if the event is being signalled
        if( theChange > 0 ) // here we can also assume event must be > 0 if theChange is > 0
            signalVal =  pthread_cond_signal( &m_condition );
        }

    retVal = pthread_mutex_unlock( &m_accessMutex );
    if ( retVal )
        NERROR( retVal, "Nevent::ChangeCountingEventState: Error unlocking access mutex." );

    // Delay throwing of exception until after we unlocked mutex just so we dont leave a locked mutex around
    if ( signalVal )
        NERROR( signalVal, "Nevent::ChangeBoolEventState: Can't signal condition." );
}


void Nevent::resetCountingEventState()
{
    int signalVal = 0;
    int retVal = pthread_mutex_lock( &m_accessMutex );

    if ( retVal )
        NERROR( retVal, "Nevent::ResetCountingEventState: Error locking access mutex." );

    m_eventUnion.m_countingEvent = 0;

    retVal = pthread_mutex_unlock( &m_accessMutex );
    if ( retVal )
        NERROR( retVal, "Nevent::ResetCountingEventState: Error unlocking access mutex." );

    // Delay throwing of exception until after we unlocked mutex just so we dont leave a locked mutex around
    if ( signalVal )
        NERROR( signalVal, "Nevent::ResetCountingEventState: Can't signal condition." );
}

