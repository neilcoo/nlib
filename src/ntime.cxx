// ntime.cxx by Neil Cooper. See ntime.h for documentation
#include "ntime.h"

#include <string.h>       // for NULL
#include <limits.h>       // for LONG_MAX
#include <sys/sysinfo.h>  // for sysinfo()
#include <sched.h>        // for sched_yield()
#include <errno.h>        // for EINTR

#include "nerror.h"

static const long  NS_IN_1_USEC        = 1000;        // Nanoseconds in 1 micreosecond
static const long  NS_IN_1_MSEC        = 1000000;     // Nanoseconds in one millisecond
static const long  NS_IN_1_SEC         = 1000000000;  // Nanoseconds in 1 second
// static const long  US_IN_1_MSEC        = 1000;        // Microseconds in 1 millisecond
// static const long  US_IN_1_SEC         = 1000000;     // Microseconds in 1 second
static const long  MS_IN_1_SEC         = 1000;        // Milliseconds in 1 second

static const long  SECS_IN_1_MINUTE    = 60;
static const long  MINUTES_IN_1_HOUR   = 60;
static const long  HOURS_IN_1_DAY      = 24;


bool Ntime::sleep( const Ntime& theDelay, Ntime* theRemainingTime )
{
    bool status = true;
    if ( theDelay.isZeroTime() )  // Sleep(0) means give up the timeslice.
        {
        int status = sched_yield();
        if ( status != 0 )
            EERROR( status, "Ntime::Sleep: sched_yield() failed." );
        }
    else
        {
        // Delay for the given number of milliseconds
        Ntime delay = theDelay;
        Ntime remaining;
        bool repeat = false;
        do
            {
            // -1 = error or we were interrupted by a signal
            status = ( nanosleep( &(delay.m_time), &(remaining.m_time) ) == 0 );
            if ( !status )
                {
                if ( errno != EINTR ) // If it wasn't a signal
                    EERROR( "Ntime::Sleep: nanosleep() failed." );

                repeat = !theRemainingTime;
                delay = remaining;
                }
            } while ( repeat );

        if ( theRemainingTime )
            *theRemainingTime = remaining;
        }

    return status;
}


Ntime Ntime::getUptime()
{
    struct sysinfo info;
    if ( sysinfo( &info ) != 0 )
        EERROR( "Ntime::GetUptime: sysinfo() failed." );

    Ntime uptime;
    uptime.setInSeconds( info.uptime );
    return uptime;
}


Ntime Ntime::getCurrentLocalTime()
{
    timespec ts;
    if ( clock_gettime( CLOCK_REALTIME, &ts ) != 0 )
        EERROR("TimeImpl::getCurrentLocalTime: clock_gettime() failed.");
    Ntime currentTime(ts);
    return currentTime;
}


const Ntime Ntime::getLocalTimeOffset()
{
    // NB Comprises of timezone and daylight savings difference
    time_t      timeNow;
    struct tm   tLocal;

    time( &timeNow );
    localtime_r( &timeNow, &tLocal );
    Ntime t;
    t.setInSeconds( tLocal.tm_gmtoff );

    return t;
}


//----------------------------------------------------------------------------
// Implementation of non-static methods
//----------------------------------------------------------------------------

Ntime::Ntime( const Ntime& ntime )
{
    m_time = ntime.m_time;
}

// Ntime class to encapulate the linux timeval and timesec structs

Ntime::Ntime( const long long theMilliSeconds )
{
    setInMs( theMilliSeconds );
}


Ntime::Ntime( const timespec& theTimespec )
{
    m_time.tv_sec  = theTimespec.tv_sec;
    m_time.tv_nsec = theTimespec.tv_nsec;
    rationalise();
}


Ntime::Ntime( const timeval& theTimeval )
{
    m_time.tv_sec  = theTimeval.tv_sec;
    m_time.tv_nsec = theTimeval.tv_usec * NS_IN_1_USEC;
    rationalise();
}


Ntime& Ntime::operator =( const Ntime& ntime )
{
    m_time = ntime.m_time;
    return *this;
}


Ntime& Ntime::operator =( const timeval& theTimeval )
{
    m_time.tv_sec  = theTimeval.tv_sec;
    m_time.tv_nsec = theTimeval.tv_usec * NS_IN_1_USEC;
    rationalise();
    return *this;
}


Ntime& Ntime::operator =( const timespec& theTimespec )
{
    m_time.tv_sec = theTimespec.tv_sec;
    m_time.tv_nsec = theTimespec.tv_nsec;
    rationalise();
    return *this;
}


Ntime& Ntime::operator +=( const Ntime& theTime )
{
    m_time.tv_nsec += theTime.m_time.tv_nsec;
    m_time.tv_sec += theTime.m_time.tv_sec;
    rationalise();
    return *this;
}


Ntime Ntime::operator +( const Ntime& theTime )
{
    Ntime r( *this );
    return r += theTime;
}


Ntime& Ntime::operator -=( const Ntime& theTime )
{
    m_time.tv_sec -= theTime.m_time.tv_sec;
    m_time.tv_nsec -= theTime.m_time.tv_nsec;
    rationalise();
    return *this;
}


Ntime Ntime::operator -( const Ntime& theTime )
{
    Ntime r( *this );
    return r -= theTime;
}


bool Ntime::operator ==( const Ntime& theTime ) const
{
    return ( ( m_time.tv_sec == theTime.m_time.tv_sec ) &&
             ( m_time.tv_nsec == theTime.m_time.tv_nsec ) );
}


bool Ntime::operator <( const Ntime& theTime ) const
{
    return ( ( m_time.tv_sec < theTime.m_time.tv_sec ) ||
             ( ( m_time.tv_sec == theTime.m_time.tv_sec ) &&
               ( m_time.tv_nsec < theTime.m_time.tv_nsec ) )
           );
}


bool Ntime::operator >( const Ntime& theTime ) const
{
    return ( ( m_time.tv_sec > theTime.m_time.tv_sec ) ||
             ( ( m_time.tv_sec == theTime.m_time.tv_sec ) &&
               ( m_time.tv_nsec > theTime.m_time.tv_nsec ) )
           );
}


bool Ntime::sleep( Ntime* theRemainingTime )
{
    return Ntime::sleep( *this, theRemainingTime );
}


bool Ntime::isZeroTime() const
{
    return ( ( m_time.tv_sec == 0 ) && ( m_time.tv_nsec == 0 ) );
}


const Ntime Ntime::getElapsed() const
{
    return getDifference( getCurrentLocalTime() );
}


const Ntime Ntime::getDifference( const Ntime& theTime ) const
{
    Ntime x = theTime;
    return x - *this;
}


void Ntime::setInMs( const long long theMillisecs )
{
    m_time.tv_sec  = theMillisecs / MS_IN_1_SEC;
    m_time.tv_nsec = ( theMillisecs % MS_IN_1_SEC ) * NS_IN_1_MSEC;
}


void Ntime::setInSeconds( const long theSecs )
{
    m_time.tv_sec  = theSecs;
    m_time.tv_nsec = 0;
}


void Ntime::setInMinutes( const long theMinutes )
{
    m_time.tv_sec  = theMinutes * SECS_IN_1_MINUTE;
    m_time.tv_nsec = 0;
}


void Ntime::setInHours( const long theHours )
{
    m_time.tv_sec  = ( theHours * SECS_IN_1_MINUTE ) * MINUTES_IN_1_HOUR;
    m_time.tv_nsec = 0;
}


void Ntime::setInDays( const long theDays )
{
    m_time.tv_sec  = ( ( theDays * SECS_IN_1_MINUTE ) * MINUTES_IN_1_HOUR ) * HOURS_IN_1_DAY;
    m_time.tv_nsec = 0;
}


const long long Ntime::getAsMs() const
{
    long long t = m_time.tv_sec;
    t = t * MS_IN_1_SEC;
    t = t + ( m_time.tv_nsec / NS_IN_1_MSEC );
    return t;
}


const long Ntime::getAsSeconds() const
{
    return ( m_time.tv_sec + ( m_time.tv_nsec / NS_IN_1_SEC ) );
}


const long Ntime::getAsMinutes() const
{
    return ( getAsSeconds() / SECS_IN_1_MINUTE );
}


const long Ntime::getAsHours() const
{
    return ( getAsMinutes() / MINUTES_IN_1_HOUR );
}


const long Ntime::getAsDays() const
{
    return ( getAsHours() / HOURS_IN_1_DAY );
}


const timeval Ntime::getAsTimeval() const
{
    timeval tv;
    tv.tv_sec  = m_time.tv_sec;
    tv.tv_usec = m_time.tv_nsec / NS_IN_1_USEC;
    return tv;
}


const timespec Ntime::getAsTimespec() const
{
    timespec ts;
    ts.tv_sec  = m_time.tv_sec;
    ts.tv_nsec = m_time.tv_nsec;
    return ts;
}


const std::string Ntime::getAsString() const
{
    static const unsigned short CTIME_R_MIN_BUFFER_LENGTH = 26;

    char tempString[ CTIME_R_MIN_BUFFER_LENGTH ];
    time_t secsSinceEpoch = getAsSeconds();

    ctime_r( &secsSinceEpoch, tempString );

    short lastCharIndex = strlen( tempString ) - 1;

    if (lastCharIndex >= 0 )
        if ( tempString[ lastCharIndex ] == '\n' )
            tempString[ lastCharIndex ] = 0;

    return tempString;
}


void Ntime::addMillisecs( const long long theMillisecs )
{
    m_time.tv_sec  += ( theMillisecs / MS_IN_1_SEC );
    m_time.tv_nsec += ( theMillisecs % MS_IN_1_SEC ) * NS_IN_1_MSEC;
    rationalise();
}


void Ntime::addSecs( const long theSecs )
{
    m_time.tv_sec += theSecs;
    rationalise();
}


void Ntime::rationalise()
{
    m_time.tv_sec  += ( m_time.tv_nsec / NS_IN_1_SEC );
    m_time.tv_nsec  = m_time.tv_nsec % NS_IN_1_SEC;

    if ( ( m_time.tv_nsec < 0 ) && ( m_time.tv_sec > 0 ) )
        {
        m_time.tv_sec -= 1;
        m_time.tv_nsec += NS_IN_1_SEC;
        }

    if ( ( m_time.tv_nsec > 0 ) && ( m_time.tv_sec < 0 ) )
        {
        m_time.tv_sec += 1;
        m_time.tv_nsec -= NS_IN_1_SEC;
        }
}

