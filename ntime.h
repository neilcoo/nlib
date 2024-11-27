#ifndef NTIME_H
#define NTIME_H

// Ntime v1.6 by Neil Cooper 11th Dec 2019
// Ntime is a helper class that encapulates a timespec struct.
// It provides lots of useful functionality related to time
// such as timespec/timeval conversions, math operators,
// delays, elapsed time, and easy access to current system time
// and local timezone.
// It also implements Win32-like GetTickCount() and Sleep().
// ( Older versions of Linux sleep() gobble cpu time. Ntime's
//  Sleep() doesn't ).

#include <time.h>     // for timespec
#include <sys/time.h> // for timeval
#include <string>     // for std::string

class Ntime
{
public:
    static bool sleep( const  Ntime& theDelay,
                       Ntime* theTimeRemaining = NULL );
    // Delays the calling thread for the time specifed in parameter 1.
    // Return value:
    // true  = Success.
    // false = Sleep was terminated early because of a signal.
    // This method's signal-handling behaviour depends on whether the
    // optional 2nd parameter is specified or not.
    // If not specified, Sleep() will ignore signals.
    // If a pointer to an Ntime object is given as the 2nd parameter,
    // a signal will cause Sleep() terminate early and store the time
    // remaining in the object pointed to. This could then be used as
    // the first parameter in a subsequent call of Sleep in order
    // to complete the originally specified pause.
    // For Win32 compatability: If Sleep is called with 0 delay, the
    // thread's current timeslice is yielded.

    static Ntime getUptime();
    // Length of time that the system has been running. Resolution is 1 second.

    static Ntime getCurrentLocalTime();
    // Get the current local time (i.e. as offset from epoch).

    static const Ntime getLocalTimeOffset();
    // Returns the offset in seconds between UTC and local time.
    // Both timezone and daylight savings time are factored in.

    Ntime( const Ntime& ntime );
    Ntime( const long long theMilliSeconds = 0 );
    Ntime( const timeval& theTimeval );
    Ntime( const timespec& theTimespec );

    virtual ~Ntime() {};

    // Following methods and operators allow operations to be performed
    // on the stored time
    Ntime& operator  =( const Ntime& theTime );
    Ntime& operator  =( const timeval& theTimeval );
    Ntime& operator  =( const timespec& theTimespec );
    Ntime& operator +=( const Ntime& theTime );
    Ntime  operator  +( const Ntime& theTime );
    Ntime& operator -=( const Ntime& theTime );
    Ntime  operator  -( const Ntime& theTime );
    bool   operator ==( const Ntime& theTime );

    void addSecs( const long theSecs );
    void addMillisecs( const long long theMillisecs );

    bool sleep( Ntime* theTimeRemaining = NULL );
    // As static Sleep() but uses the time stored in the instance as the delay.

    bool isZeroTime() const;
    // Returns true only if encapsulated time value is 0

    const Ntime getElapsed() const;
    // Returns the elapsed time from the stored time.

    const Ntime getDifference( const Ntime& theTime ) const;
    // Returns the difference between the stored time and given time.

    // Following methods allow a time to be stored in various different
    // formats
    void setInMs( const long long theMillisecs );
    void setInSeconds( const long theSecs );
    void setInMinutes( const long theMinutes );
    void setInHours( const long theHours );
    void setInDays( const long theDays );

    // Following methods allow a time to be read in various different
    // formats
    const long long     getAsMs() const;
    const long          getAsSeconds() const;
    const long          getAsMinutes() const;
    const long          getAsHours() const;
    const long          getAsDays() const;
    const timeval       getAsTimeval() const;
    const timespec      getAsTimespec() const;
    const std::string   getAsString() const;

    // GetAsString() assumes stored time is relative to the epoch.
    // The format is like the result of ctime() but with no '\n' on the end, e.g:
    // "Wed Jun 30 21:49:08 1993".

private:
    void rationalise();

    timespec m_time;
};


#endif
