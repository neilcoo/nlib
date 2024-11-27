// npsql.cxx by Neil Cooper. See npsql.h for documentation
#include "npsql.h"

#include <iostream>    // for cerr for NoticeProcessor
#include <sstream>
#include <string>
#include <libpq-fe.h>

#include "npsqlResult.h"
#include "nerror.h"
#include "ntime.h"

using namespace std;

const int PQSTATUS_WAIT_DELAY_MS  = 1;

// static member
Nmutex Npsql::m_notReentrant;

// Callback provided to PQsetNoticeProcessor
static void noticeProcessor( void* theParent, const char* theMessage )
{
    if ( ( (Npsql*)theParent )->getNoticesEnabled() )
        {
        FILE* noticeStream = ( (Npsql*)theParent )->getNoticeStream();

        if ( noticeStream )
            fprintf( noticeStream, "Npsql:%s\n", theMessage );
        else
            cerr << "Npsql:" << theMessage << endl;
        }
}

// Destructor
Npsql::~Npsql()
{
    if ( m_db )
        {
        string msg;
        waitForDb( &msg );
        PQfinish( m_db );
        }
}


Npsql::Npsql(   const char*       theDbName,
                const char*       theUser,
                unsigned short    thePort,
                const char*       theHost,
                const bool        theHostIsAnIpNo ) : m_db( NULL ),
                                                      m_notices( NULL ),
                                                      m_initialised( false ),
                                                      m_noticesEnabled( true )
{
    // Compose the parameters into a string for PQconnectdb()
    string params = "dbname=";
    params += theDbName;

    if ( theUser )
        {
        params += " user=";
        params += theUser;
        }

    if ( thePort )
        {
        char port[6];
        sprintf(port, "%u", thePort);
        params += " port=";
        params += thePort;
        }

    if ( theHost )
        {
        if ( theHostIsAnIpNo )
            params += " hostaddr=";
        else
            params += " host=";
        params += theHost;
        }

    // Attempt to connect (which is not reentrant so is protected with a mutex)
    m_notReentrant.lock();

    m_db = PQconnectdb( params.c_str() );

    m_notReentrant.unlock();

    if ( !m_db )
    ERROR( "Not enough memory to open database" );

    string msg;
    if ( !waitForDb( &msg ) )
        ERROR( msg.c_str() );

    // Set up our notice processor callback function
    PQsetNoticeProcessor( m_db, noticeProcessor, this );

    m_initialised = true;
}


bool Npsql::query(  const char*   theQuery,
                    NpsqlResult*  theResults,
                    const bool    theWarnFlag  )
{
    if (!m_initialised)
        return false;

    PGresult* result = PQexec( m_db, theQuery );

    if ( theResults )
        *theResults = result;

    bool status = false;
    bool repeat = false;

    // Wait until the query has finished
    do
        {
        repeat = false;

        switch ( PQresultStatus( result ) )
            {
            case PGRES_COMMAND_OK:
            case PGRES_TUPLES_OK:
                status = true;
                break;

            case PGRES_COPY_OUT:
            case PGRES_COPY_IN:
                repeat = true;
                Ntime::sleep( PQSTATUS_WAIT_DELAY_MS );
                break;

            case PGRES_EMPTY_QUERY:
                status = false;
                break;

            case PGRES_BAD_RESPONSE:
                ERROR( "Got incomprehensible response from database server" );
                break;

            case PGRES_NONFATAL_ERROR:
                status = false;
                if ( theWarnFlag )
                WARN( "Got nonfatal error status back from database server. Query was: '",
                      theQuery, "'" );
                break;

            case PGRES_FATAL_ERROR:
                status = false;
                if ( theWarnFlag )
                    WARN( "Got fatal error status back from database server. Query was: '",
                          theQuery, "'" );
                break;

            default:
                status = false;
                ERROR( "Unknown status return value from database server" );
            }
        } while ( repeat );

    if ( !theResults )
        PQclear( result );

    return status;
}


bool Npsql::query(  const ostringstream&  theQuery,
                    NpsqlResult*          theResults,
                    const bool            theWarnFlag )
{
    return query( theQuery.str().c_str(), theResults, theWarnFlag );
}


void Npsql::beginDebugTrace( FILE* theDebugPort )
{
    PQtrace( m_db, theDebugPort );
}


void Npsql::endDebugTrace()
{
    PQuntrace( m_db );
}


void Npsql::enableNotices( const bool theEnableNoticesFlag )
{
    m_noticesEnabled = theEnableNoticesFlag;
}


bool Npsql::getNoticesEnabled()
{
    return m_noticesEnabled;
}


void Npsql::divertNotices( FILE* theDestination )
{
    m_notices = theDestination;
}


FILE* Npsql::getNoticeStream()
{
    return m_notices;
}


bool Npsql::waitForDb( string* theMessage )
{
    // Wait until db is open or definately bad
    bool repeat = false;

    do
        {
        repeat = false;

        switch ( PQstatus( m_db ) )
            {
            case CONNECTION_OK:
                return true;
                break;

            case CONNECTION_BAD:
                {
                *theMessage = "Database connection bad. Error message = '";
                *theMessage += PQerrorMessage( m_db );
                *theMessage += "'";
                }
                break;

            case CONNECTION_STARTED:
            case CONNECTION_MADE:
            case CONNECTION_AWAITING_RESPONSE:
            case CONNECTION_AUTH_OK:
            case CONNECTION_SETENV:
                Ntime::sleep( PQSTATUS_WAIT_DELAY_MS );
                repeat = true;
                break;

            default:
                *theMessage = "Unknown status from PQstatus() while opening database";
            }
        } while ( repeat );

    return false;
}


// PostgreSQL doesnt store unsigned longs so we get around that by
// 2's complementing them manually with the following conversion functions

unsigned long Npsql::long2Ulong( const long theLong )
{
    if ( theLong < 0 )
        return ( - ( ( ~theLong ) + 1 ) );
    else
        return theLong;
}


long Npsql::ulong2Long( const unsigned long theUlong )
{
    const unsigned long PSQL_INT4_MAX = 0x7FFFFFFF;

    if ( theUlong > PSQL_INT4_MAX )
        return ( - ( ( ~theUlong ) + 1 ) );
    else
        return theUlong;
}


