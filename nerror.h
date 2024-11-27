#ifndef NERROR_H
#define NERROR_H

// Nerror v1.6 by Neil Cooper 30th December 2019
// Nerror provides a complete exception-based error-handling mechanism via 5 macros:
// HANDLE_NERRORS, NERROR_HANDLER, ERROR, WARN and LOG.
//
// HANDLE_NERRORS Simple macro to hook the NERROR_HANDLER into the unhandled
// exception mechanism provided by C++ and the Standard Template Library.
//
// NERROR_HANDLER( <function> ) Is an explicit handler used to execute the given
// function within an exception-controlled environment. Any exceptions thrown
// within will be caught and reported. If the exception is of type NerrorException,
// more detailed reporting takes place. Since Nerror v1.5, use of this macro is
// generally no longer necessary, just use HANDLE_NERRORS instead.
//
// 3 macros are provided to easily generate Log and Warning messages, and to
// remove the need to create, populate and throw an NerrorException object by hand.
//
// ERROR( <msg>... )
//    Use for fatal errors: Error logs the given message and terminates the
//    application (by exiting the error handler). Calls to ERROR do not return.
//    The message is prefixed by "Fatal Error: " when reported.
//
// EERROR( <msg>... )
//    As ERROR but appends text of current errno to end of message.
//
// NERROR( <errno>, <msg>... )
//    As ERROR but appends text of given errno to end of message.
//
// WARN( <msg> )
//    Use for non-fatal errors: The given message is logged but execution continues
//    (i.e. unlike ERROR(), WARN() does return)
//    The message is prefixed by "Warning: " when reported.
//
// EWARN( <msg> )
//    As WARN but appends text of current errno to end of message.
//
// NWARN( <errno>, <msg> )
//    As WARN but appends text of given errno to end of message.
//
// LOG( <msg> )
//    Use for reporting events or messages that are not errors or warnings.
//    Functionality is the same as WARN except that the message does not gain a prefix.
//
// ELOG( <msg> )
//    As LOG but appends text of current errno to end of message.
//
// NLOG( <msg>, <errno> )
//    As LOG but appends text of given errno to end of message.
//
// By default, all reporting from the above macros is output on the CERR stream with the
// exception that LOG messages go to COUT.
// If compiler symbol NERROR_USE_SYSLOG is defined, or NERROR::UseSysLog( true )
// was called at runtime, all subsequent output will go to the system log ( syslogd )
// instead. This allows for control of the error and logging output which
// is especially useful in the development of system applications such as daemons
// or servers. (e.g. when developing/debugging, error messages go to the terminal but
// for a release build they go to the system log).
//
// If compiler symbol DEBUG is defined, logging will additionally include file name
// and line number information from where the logging message originated ( to help
// in development/debugging ).
//
// Example usage:
// int main( const int ac, const char* av[] )
// {
//    HANDLE_NERRORS;
//
//    if ( ac > 1 )
//       ERROR( "Unexpected command line parameter(s): '", av[1], "'" );

//    // you can assume ac <= 1 here as no forms of ERROR() ( EERROR() etc ) return
//    ...
// }
//
// Example explicit handler usage (deprecated).
// Note: Using this form, each pthread will need their own instance of NERROR_HANDLER.
//
// void SafeMain( const int ac, const char* av[] )
// {
//    // Use this like a 'real' main ....
//    LOG( "Starting up" );
//
//    if ( ac > 1 )
//       WARN( "Additional command line parameter(s) ignored" );
//
//    if ( ! char* buffer = new char[999999999] )
//       EERROR( "Can't allocate buffer", errno );
//
//    // you can assume 'buffer' is allocated safely here
//    ...
// }
//
// int main( const int ac, const char* av[] )
// {
//    NERROR_HANDLER( SafeMain( ac, av ) );
// }
//
// End of example usage

#include <stdlib.h>  // for exit()
#include <string>
#include <sstream>   // for ostringstream
#include <syslog.h>  // for syslog()
#include <iostream>  // for cout, cerr
#include <errno.h>   // for errno
#include <exception> // for std::set_terminate()

class NerrorException : public std::exception
{
public:
    NerrorException( const std::string&   errorMsg,
                     const char*          fileName,
                     unsigned long        lineNo,
                     const int            theErrno );

    virtual ~NerrorException() {};

    virtual const char*     ErrorMessage() { return m_msg.c_str(); }
    virtual const char*     FileName()     { return m_fileName.c_str(); }
    virtual unsigned long   LineNo()       { return m_lineNo; }
    virtual const int       Errno()        { return m_errno; }

    // Allow std::exception to use our message
    const char* what() const noexcept { return m_msg.c_str(); }

private:
    std::string    m_msg;
    std::string    m_fileName;
    unsigned long  m_lineNo;
    int            m_errno;
};


#ifdef DEBUG
#define LOG( msg... ) Nerror::Log( Nerror::Compose( msg ), __FILE__, __LINE__, 0 )

#define ELOG( msg... ) Nerror::Log( Nerror::Compose( msg ), __FILE__, __LINE__, errno )

#define NLOG( err, msg... ) Nerror::Log( Nerror::Compose( msg ), __FILE__, __LINE__, err )

#define WARN( msg... ) Nerror::Warn( Nerror::Compose( msg ), __FILE__, __LINE__, 0 )

#define EWARN( msg... ) Nerror::Warn( Nerror::Compose( msg ), __FILE__, __LINE__, errno )

#define NWARN( err, msg... ) Nerror::Warn( Nerror::Compose( msg ), __FILE__, __LINE__, err )

// We need to explicitly call exit to stop NERROR_HANDLER just returning after fatal errors.
// This can be dangerous especially in multithreaded environment where a fatal error just
// would terminate a thread otherwise.

#define NERROR_HANDLER( guarded )                                               \
{                                                                               \
    try { guarded; }                                                            \
    catch( NerrorException& error )                                             \
        {                                                                       \
        if ( Nerror::IsUsingSysLog() )                                          \
            syslog( LOG_CRIT | LOG_ERR,                                         \
                    "Fatal Error: %s line %lu: %s",                             \
                    error.FileName(),                                           \
                    error.LineNo(),                                             \
                    error.ErrorMessage() );                                     \
        else                                                                    \
            std::cerr << "Fatal Error: " << error.FileName() <<                 \
            " line " << error.LineNo() << ": "                                  \
            << error.ErrorMessage() << std::endl;                               \
        exit( EXIT_FAILURE );                                                   \
        }                                                                       \
    catch( std::exception& error )                                              \
        {                                                                       \
        if ( Nerror::IsUsingSysLog() )                                          \
            syslog( LOG_CRIT | LOG_ERR,                                         \
                    "Fatal Error: %s",                                          \
                    error.what() );                                             \
        else                                                                    \
            std::cerr << "Fatal Error: " << error.what() << std::endl;          \
        exit( EXIT_FAILURE );                                                   \
        }                                                                       \
    catch(...)                                                                  \
        {                                                                       \
        if ( Nerror::IsUsingSysLog() )                                          \
            syslog( LOG_CRIT | LOG_ERR,                                         \
                    "Fatal Error: Caught unknown exception" );                  \
        else                                                                    \
            std::cerr << "Fatal Error: Caught unknown exception" << std::endl;  \
        exit( EXIT_FAILURE );                                                   \
        }                                                                       \
}

#else   // ifdef DEBUG

#define LOG( msg... ) Nerror::Log( Nerror::Compose( msg ), 0 )

#define ELOG( msg... ) Nerror::Log( Nerror::Compose( msg ), errno )

#define NLOG( err, msg... ) Nerror::Log( Nerror::Compose( msg ), err )

#define WARN( msg... ) Nerror::Warn( Nerror::Compose( msg ), 0 )

#define EWARN( msg... ) Nerror::Warn( Nerror::Compose( msg ), errno )

#define NWARN( err, msg... ) Nerror::Warn( Nerror::Compose( msg ), err )


#define NERROR_HANDLER( guarded )                                               \
{                                                                               \
    try { guarded; }                                                            \
    catch( NerrorException& error )                                             \
        {                                                                       \
        if ( Nerror::IsUsingSysLog() )                                          \
            syslog( LOG_CRIT | LOG_ERR,                                         \
                     "Fatal Error: %s",                                         \
                    error.ErrorMessage() );                                     \
        else                                                                    \
            std::cerr << "Fatal Error: " << error.ErrorMessage() << std::endl;  \
        exit( EXIT_FAILURE );                                                   \
        }                                                                       \
    catch( std::exception& error )                                              \
        {                                                                       \
        if ( Nerror::IsUsingSysLog() )                                          \
            syslog( LOG_CRIT | LOG_ERR,                                         \
                     "Fatal Error: %s",                                         \
                    error.what() );                                             \
        else                                                                    \
            std::cerr << "Fatal Error: " << error.what() << std::endl;          \
        exit( EXIT_FAILURE );                                                   \
        }                                                                       \
    catch(...)                                                                  \
        {                                                                       \
        if ( Nerror::IsUsingSysLog() )                                          \
            syslog( LOG_CRIT | LOG_ERR,                                         \
                   "Fatal Error: Caught unknown exception" );                   \
        else                                                                    \
            std::cerr << "Fatal Error: Caught unknown exception" << std::endl;  \
        exit( EXIT_FAILURE );                                                   \
        }                                                                       \
}

#endif  // ifdef DEBUG

#define ERROR( msg... ) throw( NerrorException( Nerror::Compose( msg ), __FILE__, __LINE__, 0 ) )

#define EERROR( msg... ) throw ( NerrorException( Nerror::Compose( msg ), __FILE__, __LINE__, errno ) )

#define NERROR( err, msg... ) throw ( NerrorException( Nerror::Compose( msg ), __FILE__, __LINE__, err ) )

#define HANDLE_NERRORS std::set_terminate( Nerror::TerminateHandler )

class Nerror
{
public:
    static void UseSysLog( const bool theUseSysLogFlag );
    // Controls message destination from the error handler and ERROR/WARN/LOG macros.
    // Parameter: true = syslogd, false = CERR/COUT

    static bool IsUsingSysLog();
    // Returns current message destination from the error handler and ERROR/WARN/LOG macros.
    // return: true = syslogd, false = CERR/COUT

    // Implementation in header so we can use DEBUG conditional compilation in the app
    // to control level of diagnostic ouput from NERROR_HANDLER even when Nlib has
    // been built without it.
    static void TerminateHandler()
        { NERROR_HANDLER( std::rethrow_exception( std::current_exception() ) ); }

    template <typename ...Msgs>
    static std::string Compose( Msgs&&... msgs )
        {
        std::ostringstream* mBuf = new std::ostringstream;
        std::string msg = Compose2( *mBuf, msgs... );
        delete mBuf;
        return msg;
        }

    static void Warn( const std::string&   theMessage,
                      const char*          theFilename,
                      const unsigned long  theLineNo,
                      const int            theErrno     );

    static void Warn( const std::string&  theMessage,
                      const int           theErrno      );

    static void Log( const std::string&     theMessage,
                     const char*            theFilename,
                       const unsigned long  theLineNo,
                       const int            theErrno    );

    static void Log( const std::string& theMessage,
                     const int          theErrno    );

private:
    template <typename First, typename ...Rest>
    static std::string Compose2( std::ostringstream& msgBuf, const First& first, Rest&&... Args )
        {
        msgBuf << first;
        Nerror::Compose2( msgBuf, Args... );
        return msgBuf.str();
        }

    static std::string Compose2( std::ostringstream& msgBuf );
    static bool useSysLog;
};

#endif

