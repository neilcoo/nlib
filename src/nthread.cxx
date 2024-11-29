// nthread.cxx by Neil Cooper. See nthread.h for documentation
#include "nthread.h"

#include <errno.h>    // for ESRCH
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <thread>
#include <sstream>

#include "nerror.h"
#include "ntime.h"

using namespace std;

#ifndef __ANDROID__
const Nthread::CORE_AFFINITY Nthread::CORE_AFFINITY_ALL = "0b" + string( std::thread::hardware_concurrency(), '1' );
#endif

static const Ntime DEATH_WAIT_TIME_MS( 1000 );   // Time (in ms) we allow thread to terminate


void* Nthread::getTidAndCall( void* theParams )
{
    CallerParams* params = (CallerParams*)theParams;

    long tid = syscall( SYS_gettid ); // after ubuntu 18.04 can use gettid();
    if ( tid == -1 )
        EERROR("Nthread: syscall(SYS_gettid) failed.");

    *(params->tid) = tid;
    params->tidGot->signal();
    void* retVal = params->threadProc( params->threadParams );
    return retVal;
}


Nthread::Nthread( NTHREAD_THREAD_PROC  theProcess,
                  void*                theParameter,
                  const string         theName,
                  const bool           theCreateDetachedFlag ) :  m_created( false ),
                                                                  m_detached( theCreateDetachedFlag ),
                                                                  m_joined( false ),
                                                                  m_tidGot( false, 0, true )

{
    pthread_attr_t  attributes;

    int status = pthread_attr_init( &attributes );
    if ( status )
        NERROR( status, "Nthread::Nthread Can't set default thread attributes" );

    status = pthread_attr_setdetachstate( &attributes, m_detached );
    if ( status )
        NERROR( status, "Nthread::Nthread Can't set Detach state" );

    m_callerParams.threadProc = theProcess;
    m_callerParams.threadParams = theParameter;
    m_callerParams.tid = &m_tid;
    m_callerParams.tidGot = &m_tidGot;

    status = pthread_create( &m_threadId, &attributes, getTidAndCall, (void*)&m_callerParams );

    if ( status )
        NERROR( status, "Nthread::Nthread pthread_create() failed." );

    m_created = true;

/* For playing around with KillThread()
   status = pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
   if ( status )
      NERROR( status, "Nthread::Nthread pthread_setcanceltype() failed." );
*/

    if ( theName.length() )
        status = pthread_setname_np( m_threadId, theName.c_str() );

    if ( status )
        NERROR( status, "Nthread::Nthread pthread_setname_np() failed." );

    // Destroy the attributes. Only for portability/future-proofing,
    // ( currently does nothing in Linux Pthreads ).
    status = pthread_attr_destroy( &attributes );
    if ( status )
        NERROR( status, "Nthread::Nthread pthread_attr_destroy() failed." );
}


Nthread::~Nthread()
{
    if ( m_created )
        killThread();
}


pid_t Nthread::getTid()
{
    m_tidGot.wait(); // wait until we have the tid
    // m_tidGot is a manual reset event so wait will always drop through after set once
    return m_tid;
}


void Nthread::setNice( int priority ) // -20 highest, 19 lowest
{
    if ( setpriority( PRIO_PROCESS, getTid(), priority ) == -1 )
        EERROR("Nthread::setNice Can't set thread priority" );
}


int Nthread::getNice()
{
    errno = 0;
    int priority = getpriority( PRIO_PROCESS, getTid() );
    if ( ( priority == -1 ) && ( errno != 0 ) )
        EERROR("Nthread::getNice Can't get thread priority" );

    return priority;
}

#ifndef __ANDROID__
void Nthread::setSchedulingPriority( const int thePriority )
{
    int status = pthread_setschedprio( m_threadId, thePriority );
    if ( status != 0 )
        NERROR( status, "Nthread::setSchedulingPriority can't set thread priority" );
}


int Nthread::getSchedulingPriority()
{
    int policy = 0;
    struct sched_param param;
    int status = pthread_getschedparam( m_threadId, &policy, &param );

    if ( status != 0 )
        NERROR( status, "Nthread::getSchedulingPriority can't get thread parameters" );

    return param.sched_priority;
}
#endif


#ifndef __ANDROID__
void Nthread::setThreadAffinity( const CORE_AFFINITY allowedCores )
{
    unsigned long long flags = allowedCores.getAsInt();
    if ( flags == 0 )
        flags = CORE_AFFINITY_ALL.getAsInt();
    cpu_set_t cpus;
    CPU_ZERO( &cpus );
    unsigned short core = 0;

    while ( flags )
        {
        if ( flags & 1 )
            CPU_SET( core, &cpus );
        flags = flags >> 1;
        core++;
        }

    int status = pthread_setaffinity_np( m_threadId, sizeof(cpu_set_t), &cpus);
    if ( status )
        NERROR( status, "Nthread: Could not set thread affinity" );
}


Nthread::CORE_AFFINITY Nthread::getThreadAffinity()
{
    cpu_set_t cpus;
    CPU_ZERO( &cpus );

    int status = pthread_getaffinity_np( m_threadId, sizeof(cpu_set_t), &cpus);
    if ( status )
        NERROR( status, "Nthread: Could not get thread affinity" );

    ostringstream cores;

   int core = CPU_SETSIZE;

    while ( core > 0 )
        {
        core--;
        cores << ( CPU_ISSET( core, &cpus ) ? "1" : "0" );
        }

    CORE_AFFINITY retVal;
    retVal.setByBinary( cores.str() );

    return retVal;
}
#endif


void Nthread::setSchedulingModel( const SCHEDULING_MODEL theModel, const int thePriority  )
{
    int policy = 0;
    int priority = thePriority;

    switch( theModel )
        {
        case DEFAULT:
            policy = SCHED_OTHER;
            priority = 0;  // pthread_setschedparam requires this to be 0 for this model
            break;

        case BATCH:
            policy = SCHED_BATCH;
            priority = 0;  // pthread_setschedparam requires this to be 0 for this model
            break;

        case IDLE:
            policy = SCHED_IDLE;
            priority = 0;  // pthread_setschedparam requires this to be 0 for this model
            break;

        case FIFO:
            policy = SCHED_FIFO;
            break;

        case ROUND_ROBIN:
            policy = SCHED_RR;
            break;

        case DEADLINE:
            policy = SCHED_DEADLINE;
            break;

        default:
            ERROR( "Nthread::setSchedulingModel: Unknown scheduling model: ", theModel );
      }

    struct sched_param param;
    param.sched_priority = priority;

    int status = pthread_setschedparam( m_threadId, policy, &param);
    if ( status != 0 )
        NERROR( status, "pthread_getschedparam() failed" );
}


Nthread::SCHEDULING_MODEL Nthread::getSchedulingModel()
{
    int policy = 0;
    struct sched_param param;
    int status = pthread_getschedparam( m_threadId, &policy, &param );

    if ( status != 0 )
        NERROR( status, "Nthread::getSchedulingModel can't get thread parameters" );

    SCHEDULING_MODEL model = DEFAULT;
    switch( policy )
        {
        case SCHED_OTHER:
            model = DEFAULT;
            break;

        case SCHED_BATCH:
            model = BATCH;
            break;

        case SCHED_IDLE:
            model = IDLE;
            break;

        case SCHED_FIFO:
            model = FIFO;
            break;

        case SCHED_RR:
            model = ROUND_ROBIN;
            break;

        case SCHED_DEADLINE:
            model = DEADLINE;
            break;

        default:
            ERROR( "pthread_getschedparam() returned unknown scheduling model: ", policy );
      }

    return model;
}


void Nthread::getReturnValue( void** theReturnParam )
{
    // Can't get return value for detached threads
    int status = pthread_join( m_threadId, theReturnParam );

    if ( status != 0 )
        NERROR( status, "Nthread::getReturnValue: Can't join thread" );

    m_joined = true;
}


void Nthread::killThread()
{
    // Give thread a chance to shut down nicely, but it will only work if the user
    // hasn't set the cancel state to DISABLED ( i.e. with pthread_setcancelstate() )
    // or if the user has made calls to functions with cancellation points or calls
    // pthread_testcancel() in their code, or has called set the cancel type as
    // PTHREAD_CANCEL_ASYNCHRONOUS ( i.e. with pthread_setcanceltype() )
    // so that cancel will work immediately and without calling cancellation points.
    // NB. We can't use pthread_kill() with SIGTERM here as that stops the whole
    // process, not just the thread it is called on.

#ifndef __ANDROID__
    bool status = false;
    bool found = true;
#endif

/* Neil: commented out for now: pthread_cancel seems to screw the main thread up
    int cancelStatus = pthread_cancel( m_threadId ); // send cancel

    switch( cancelStatus )
        {
        case 0:     // Thread found and cancelled
            status = true;
            break;

        case ESRCH: // No matching thread found;
            found = false;
            break;

        default:
            NERROR( status, "Nthread::KillThread: pthread_cancel() failed." );
        }
*/

#ifndef __ANDROID__ // Android doesn't have a pthread_timedjoin
    // Need to join unjoined undetached threads to prevent zombie threads.
    // Can't join detached or previously joined threads though.
    if ( found && !m_joined && !m_detached )
        {
        Ntime delay = Ntime::getCurrentLocalTime() + DEATH_WAIT_TIME_MS;
        struct timespec timeout = delay.getAsTimespec();

        int joinStatus = pthread_timedjoin_np( m_threadId, NULL, &timeout );
        switch ( joinStatus )
            {
            case 0:           // Joined OK
                m_joined = true;
                break;

            case ETIMEDOUT:   // Timeout passed before thread exited
                {
                // all we can do is detach the thread so that its storage can be reclaimed
                // if/when the thread finally does exit.
                int detachStatus = pthread_detach( m_threadId );
                if ( detachStatus != 0 )
                    NERROR( detachStatus, "Nthread::killThread: pthread_detach() failed." );
                }
                break;

            case ESRCH:   // Thread doesn't exist
#ifdef DEBUG
                LOG( "killThread: pthread_timedjoin_np() returned ESRCH (Thread doesn't exist)" );
#endif
                break;

            case EBUSY:   // Already being joined
#ifdef DEBUG
                LOG( "killThread: pthread_timedjoin_np() returned EBUSY (Already being joined)" );
#endif
                break;

            case EDEADLK: // trying to join calling thread
#ifdef DEBUG
                LOG( "killThread: pthread_timedjoin_np() returned EDEADLK (Trying to join calling thread)" );
#endif
                break;

            case EFAULT:  // A fault occurred trying to access the buffers provided
#ifdef DEBUG
                LOG( "killThread: pthread_timedjoin_np() returned EFAULT(A fault occurred trying to access the buffers provided)" );
#endif
                break;

            case EINVAL:  // Thread isn't joinable
#ifdef DEBUG
                LOG( "killThread: pthread_timedjoin_np() returned EINVAL (Thread isn't joinable)" );
#endif
                break;

            default:
                NERROR( status, "Nthread::KillThread: pthread_timedjoin_np() failed." );
                break;
            }
        }
#endif // ifndef __ANDROID__
}

