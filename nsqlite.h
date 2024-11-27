#ifndef NSQLITE_H
#define NSQLITE_H

// Nsqlite v1.0 by Neil Cooper 14th January 2005
// Implements an easy-to-use generic SQLite version 3 database client.
// class Nsqlite encapsulates a single open SQLite database.
// Nsqlite throws exceptions of type NerrorException (see nerror.h)
// if non-recoverable errors occur.

#include <sqlite3.h>
#include "nsqliteResult.h"

class Nsqlite
{
public:

    Nsqlite( const char* theDbFile );
    //  Constructor
    //  Parameters:
    //  theDbFile = Filename of the database to create/open.

    virtual ~Nsqlite();

    bool query( const char*     theQuery,
                NsqliteResult*  theResults = NULL,
                const bool      theWarnFlag = false  );

    bool query( const std::ostringstream& theQuery,
                NsqliteResult*            theResults = NULL,
                const bool                theWarnFlag = false );

    //  Submits the given SQL statement to the database for execution
    //  Parameters:
    //        theQuery    = SQL statement to submit
    //        theResults  = Optional NsqliteResult object to receive the results of the query
    //        theWarnFlag = Ouput WARN messages for failed queries if true (default)
    //    Return:
    //        true = success, false = query failed

private:
    sqlite3*  m_db;
};


#endif
