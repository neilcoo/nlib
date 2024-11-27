#ifndef NPSQL_H
#define NPSQL_H

// Npsql v1.0 by Neil Cooper 8th March 2004
// Implements an easy-to-use generic postgreSQL database object.
// class Npsql encapsulates a single open PostgreSql database.
// Npsql throws exceptions of type NerrorException (defined in nerror.h)
// if errors occur.

#include <string>
#include <sstream>
#include "nmutex.h"
#include "npsqlResult.h"

class Npsql
{
public:

    Npsql(  const char*     theDbName,
            const char*     theUser         = NULL,
            unsigned short  thePort         = 0,
            const char*     theHost         = NULL,
            const bool      theHostIsAnIpNo = false );
    //    Constructor
    //    Parameters:
    //        theDbname       = Name of the database to open
    //        theUser         = User name to run as
    //        thePort         = TCP port to database server ( not needed if local )
    //        theHost         = Hostname of database server ( not needed if local )
    //        theHostIsAnIpNo = true if theHost parameter is specified as tcp address (e.g. 127.0.0.l)


    virtual ~Npsql();


    bool query( const char*   theQuery,
                NpsqlResult*  theResults = NULL,
                const bool    theWarnFlag = true  );


    bool query( const std::ostringstream&   theQuery,
                NpsqlResult*                theResults = NULL,
                const bool                  theWarnFlag = true );

    //    Submits the given SQL statement to the database for execution
    //    Parameters:
    //        theQuery    = SQL statement to submit
    //        theResults    = Optional NpsqlResult object to receive the results of the query
    //        theWarnFlag    = Logs failed queries if true (default)
    //    Return:
    //        true = success, false = query failed


    void beginDebugTrace( FILE* theDebugPort );
    //    Turn on debug logging to the given (already opened) file descriptor
    //    Parameters:
    //        theDebugPort    = SQL statement to submit


    void endDebugTrace();
    //    Turn off debug logging started with BeginDebugTrace()


    void enableNotices( const bool theIgnoreNoticesFlag );
    //    Control whether notices are generated or not ( default is enabled ).
    //    Parameters:
    //        true = Enable notices, false = disable notices


    bool getNoticesEnabled();
    //    Returns notices enabled status as set by EnableNotices.
    //    true = notices enabled, false = notices disabled.


    void divertNotices( FILE* theDestination );
    //    Divert the notices that the SQL backend generates to the
    //    given (Already opened) file descriptor
    //    Parameters:
    //        theDebugPort    = SQL statement to submit


    void endDivertNotices();
    //    Turn off notice diversion initiated with DivertNotices()

    FILE* getNoticeStream();
    //    Returns the stream pointer to notice stream set by DivertNotices(),
    //    or NULL if not already set

    // PostgreSQL doesnt store unsigned longs so we can get around that by
    // 2's complementing them manually with the following conversion functions

    static unsigned long long2Ulong( const long theLong );

    static long ulong2Long( const unsigned long theUlong );

private:
    PGconn*        m_db;
    FILE*          m_notices;
    bool           m_initialised;
    bool           m_noticesEnabled;
    static Nmutex  m_notReentrant;

    bool waitForDb( std::string* theMessage );
};


#endif
