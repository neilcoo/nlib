#ifndef NPSQLRESULT_H
#define NPSQLRESULT_H

// NpsqlResult v1.0 by Neil Cooper 8th March 2004
// Implements an easy-to-use generic postgreSQL database response object.
// Intended to be used with Npsql.

#include <libpq-fe.h>


class NpsqlResult
{
public:
    NpsqlResult();
    virtual ~NpsqlResult();

    NpsqlResult( PGresult* theResult );
    void operator = ( PGresult* theResult );
    //  Copy constructor and assignment operator for ease-of-use with
    //  native PostgreSQL query result type.

    const char* errorMessage();
    //  Returns the error message associated with the query,
    //  or an empty string if there was no error.

    int getRowCount();
    //  Returns the number of rows (tuples) in the result.

    int getColumnCount();
    //  Returns the number of columns (fields) in the result.

    const char* getColumnNameFromIndex( int theColumnNo = 0 );
    //  Returns the name associated with the given field index.
    //  Parameter:
    //      0-based index of column to get name of.

    int getColumnIndexFromName( const char* theColumnName );
    //  Returns the index associated with the given field name.
    //  Parameter:
    //      Field name to get index of.

    int getColumnLength( const int theColumnNo = 0 );
    //  Returns the length in bytes of the field with the given index.
    //  Parameter:
    //      0-based index of column to get length of.

    const char* getData( const int theRow = 0, const int theColumn = 0);
    //  Returns a pointer to storage containing the attribute value of
    //  the field with the given tuple and field index.
    //  The data is a character string representation (e.g. ASCII)
    //  unless DataIsBinary() returns true.
    //  NB. the returned memory location should not be written to
    //  and should only be assumed to be accessable until this
    //  NpsqlResult object is either destroyed or re-used.
    //  Parameters:
    //      0-based coordinates of attribute to get.


    bool isDataNull( const int theRow = 0, const int theColumn = 0 );
    //  Returns true if the given attribute has a NULL value.
    //  Parameters:
    //      0-based coordinates of attribute to test.

    bool dataIsBinary();
    //  Returns true if the results are in binary rather than
    //  character string representation.

    int getDataLength( const int theRow = 0, const int theColumn = 0 );
    //  Returns the length in bytes of the given attribute.
    //  Parameters:
    //      0-based coordinates of attribute to test.

    const char* getCommandStatus();
    //  Returns the command status string from the SQL command that
    //  generated the result.

    unsigned long getRowsAffectedCount();
    // Returns the number of rows (tuples) affected by the query that
    // resulted in this result set.

    void dumpResult();
    // Aid to debugging and development.
    // Just prints out the status of the result object.

private:
    void closeResult();

PGresult*    m_result;
};

#endif
