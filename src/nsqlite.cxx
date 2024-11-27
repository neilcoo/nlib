// nsqlite.cxx by Neil Cooper. See nsqlite.h for documentation
#include "nsqlite.h"

#include <iostream>
#include "nerror.h"
#include "ntime.h"

using namespace std;


static const Ntime BUSY_RETRY_DELAY_MS = 1;

Nsqlite::Nsqlite( const char* theDbFile )
{
    if ( sqlite3_open( theDbFile, &m_db ) != SQLITE_OK )
        {
        string errMsg = sqlite3_errmsg( m_db );
        sqlite3_close( m_db );
        ERROR( errMsg );
        }
}


Nsqlite::~Nsqlite()
{
    sqlite3_close( m_db );
}


bool Nsqlite::query(  const std::ostringstream& theQuery,
                      NsqliteResult*            theResults,
                      const bool                theWarnFlag )
{
    return query( theQuery.str().c_str(), theResults, theWarnFlag );
}


bool Nsqlite::query(  const char*     theQuery,
                      NsqliteResult*  theResults,
                      const bool      theWarnFlag )
{
    char* errMsg = NULL;
    int status;

    if ( theResults )
        theResults->clear();

    do
        {
        if ( theResults )
            status = sqlite3_exec( m_db, theQuery, NsqliteResult::callback, theResults, &errMsg );
        else
            status = sqlite3_exec( m_db, theQuery, NULL, NULL, &errMsg );

        if ( status == SQLITE_BUSY )
            Ntime::sleep( BUSY_RETRY_DELAY_MS );

        } while ( status == SQLITE_BUSY );

    if ( theResults )
        theResults->setRowsAffectedCount( sqlite3_changes( m_db ) );

    if( status != SQLITE_OK )
        {
        if ( errMsg )
            {
            if ( theResults )
                theResults->setErrorMessage( errMsg );

            if ( theWarnFlag )
                {
                WARN( "Query '", theQuery, "' failed: ", errMsg );
                sqlite3_free( errMsg );
                }
            else
                sqlite3_free( errMsg );
            }
        }

    return ( status ==  SQLITE_OK );
    }

