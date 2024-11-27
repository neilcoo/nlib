// nmutex.cxx by Neil Cooper. See nmutex.h for documentation
#include "nmutex.h"

#include <errno.h>	// for EBUSY

#include "nerror.h"	// for Nlib error handlers
#include "ntime.h"	// for Sleep()

// Some arbitrarary amount of time before we retry pthread_mutex_destroy
const unsigned long DESTROY_FAIL_RETRY_DELAY_MS = 10;


Nmutex::Nmutex( MUTEX_TYPE theType )
{
    // Set up the attributes of our mutex
    pthread_mutexattr_t attributes;
    int retVal = pthread_mutexattr_init(&attributes);
    if (retVal)
        NERROR(retVal, "Mutex::Mutex: Can't init mutex attributes");

    retVal = pthread_mutexattr_settype(&attributes, theType);
    if (retVal)
        NERROR(retVal, "Mutex::Mutex: Can't set type of mutex");

    retVal = pthread_mutex_init(&m_mutex, &attributes);
    if (retVal)
        NERROR(retVal, "Mutex::Mutex: Can't init mutex");

    retVal = pthread_mutexattr_destroy(&attributes);
    if (retVal)
        NERROR(retVal, "Mutex::Mutex: Can't destroy mutex attributes");
}


Nmutex::~Nmutex()
{
    // We can't destroy a locked (in use by another thread)
    // mutex so wait until we can then do it.
    while ( pthread_mutex_destroy( &m_mutex ) == EBUSY )
        Ntime::sleep( DESTROY_FAIL_RETRY_DELAY_MS );
}


void Nmutex::lock()
{
    int retVal = pthread_mutex_lock( &m_mutex );
    if ( retVal )
        NERROR( retVal, "Nmutex::Nmutex: Can't lock mutex" );
}


bool Nmutex::tryLock()
{
    int retVal = pthread_mutex_trylock( &m_mutex );
    if ( ( retVal > 0 ) && ( retVal != EBUSY ) )
        NERROR( retVal, "Nmutex::Nmutex: Can't trylock mutex" );

    return ( retVal != EBUSY );
}


void Nmutex::unlock()
{
    int retVal = pthread_mutex_unlock( &m_mutex );
    if ( retVal )
        NERROR( retVal, "Nmutex::Nmutex: Can't unlock mutex" );
}

