// nsqliteResult.cxx by Neil Cooper. See nsqliteResult.h for documentation
#include "nsqliteResult.h"

#include <iostream>         // for cout
#include <stdlib.h>         // for atol()

using namespace std;

NsqliteResult::NsqliteResult() : m_rowsAffectedCount( 0 )
{
}

NsqliteResult::~NsqliteResult()
{
}


const char* NsqliteResult::getErrorMessage()
{
    return m_errorMessage.c_str();
}


unsigned long NsqliteResult::getRowCount()
{
    return m_data.size();
}


unsigned long NsqliteResult::getColumnCount()
{
    if  ( m_data.size() )
        return m_data[0].size();
    else
        return 0;
}


const char* NsqliteResult::getColumnNameFromIndex( const unsigned long theColumnNo )
{
    return m_columnNames[theColumnNo].c_str();
}


const char* NsqliteResult::getData( const unsigned long theRow,
                                    const unsigned long theColumn )
{
    return m_data[ theRow ][ theColumn ].value.c_str();
}


bool NsqliteResult::isDataNull( const unsigned long theRow,
                                const unsigned long theColumn )
{
    return m_data[ theRow ][ theColumn ].isNull;
}


unsigned long NsqliteResult::getDataLength( const unsigned long theRow,
                                            const unsigned long theColumn )
{
    return m_data[ theRow ][ theColumn ].value.length();
}


unsigned long NsqliteResult::getRowsAffectedCount()
{
    return m_rowsAffectedCount;
}


void NsqliteResult::setErrorMessage( const char* theErrorMessage )
{
    m_errorMessage = theErrorMessage;
}


void NsqliteResult::setRowsAffectedCount( unsigned long theCount )
{
    m_rowsAffectedCount = theCount;
}


void NsqliteResult::clear()
{
    m_data.clear();
    m_columnNames.clear();
    m_errorMessage.erase();
    m_rowsAffectedCount = 0;
}


// Private Interface follows:

void NsqliteResult::dumpResult()
//Just useful for debugging
{
    cout << "Row count = " << getRowCount() << endl;
    cout << "Column count = " << getColumnCount() << endl;
    cout << "Rows affected = " << getRowsAffectedCount() << endl;
    cout << "Error Message = '" << getErrorMessage() << "'" << endl;
}


int NsqliteResult::callback(  void*   theUserParam,
                              int     fieldCount,
                              char**  fields,
                              char**  colNames       )
{
    NsqliteResult* result = (NsqliteResult*)theUserParam;
    bool initNames = ( result->getColumnCount() == 0 );

    NSQLITERESULT_DB_ROW row;

    for( int i = 0; i < fieldCount; i++ )
        {
        if ( initNames )
            result-> m_columnNames.push_back( colNames[i] );

        NSQLITERESULT_DB_FIELD field;
        field.isNull = ( !fields[i] );

        if ( !field.isNull )
            field.value = fields[i];

        row.push_back( field );
        }

    result->m_data.push_back( row );

    return 0; // return non-0 to abort early
}
