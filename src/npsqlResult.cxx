// npsqlresult.cxx by Neil Cooper. See npsqlresult.h for documentation
#include "npsqlResult.h"

#include <iostream>         // for cout
#include <stdlib.h>         // for atol()

using namespace std;

NpsqlResult::NpsqlResult() : m_result( NULL )
{
}


NpsqlResult::NpsqlResult( PGresult* theResult )
{
    closeResult();
    m_result = theResult;
}


void NpsqlResult::operator = ( PGresult* theResult )
{
    closeResult();
    m_result = theResult;
}


NpsqlResult::~NpsqlResult()
{
    closeResult();
}


const char* NpsqlResult::errorMessage()
{
    return PQresStatus( PQresultStatus( m_result ) );
}


int NpsqlResult::getRowCount()
{
    if ( PQresultStatus( m_result ) != PGRES_TUPLES_OK )
        return 0;

    return PQntuples( m_result );
}


int NpsqlResult::getColumnCount()
{
    if ( PQresultStatus( m_result ) != PGRES_TUPLES_OK )
        return 0;

    return PQnfields( m_result );
}


const char* NpsqlResult::getColumnNameFromIndex( const int theColumnNo )
{
    return PQfname( m_result, theColumnNo );
}


int NpsqlResult::getColumnIndexFromName( const char* theColumnName )
{
    return PQfnumber( m_result, theColumnName );
}


int NpsqlResult::getColumnLength( const int theColumnNo )
{
    return PQfsize( m_result, theColumnNo );
}


const char* NpsqlResult::getData( const int theRow, const int theColumn )
{
    return PQgetvalue( m_result, theRow, theColumn );
}


bool NpsqlResult::isDataNull( const int theRow, const int theColumn )
{
    return ( PQgetisnull( m_result, theRow, theColumn ) != 0 );
}


bool NpsqlResult::dataIsBinary()
{
    return ( PQbinaryTuples( m_result ) != 0 );
}


int NpsqlResult::getDataLength( const int theRow, const int theColumn )
{
    return PQgetlength( m_result, theRow, theColumn );
}


const char* NpsqlResult::getCommandStatus()
{
    return PQcmdStatus( m_result );
}


unsigned long NpsqlResult::getRowsAffectedCount()
{
    return atol( PQcmdTuples( m_result ) );
}


void NpsqlResult::closeResult()
{
    if ( m_result )
        PQclear( m_result );
}


void NpsqlResult::dumpResult()
//Just useful for debugging
{
    cout << "Command status = '" << getCommandStatus() << "'" << endl;
    cout << "Rows affected = " << getRowsAffectedCount() << endl;
    cout << "Error Message = '" <<  errorMessage() << "'" << endl;
    cout << "Row count = " << getRowCount() << endl;
    cout << "Column count = " << getColumnCount() << endl;
}

