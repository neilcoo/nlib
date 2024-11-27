#ifndef NSQLITERESULT_H
#define NSQLITERESULT_H

// NsqliteResult v1.0 by Neil Cooper 14th Jaunuary 2005
// Implements an easy-to-use generic SQLite database response object.
// Intended to be used with Nsqlite.

#include <vector>
#include <string>

class NsqliteResult
{
public:
    NsqliteResult();
    virtual ~NsqliteResult();

    const char* getErrorMessage();
    //  Returns the error message associated with the query,
    //  or an empty string if there was no error.

    unsigned long getRowCount();
    //  Returns the number of rows (tuples) in the result.

    unsigned long getColumnCount();
    //  Returns the number of columns (fields) in the result.

    const char* getColumnNameFromIndex( unsigned long  theColumnNo = 0 );
    //  Returns the name associated with the given field index.
    //  Parameter:
    //      0-based index of column to get name of.

    const char* getData(    const unsigned long  theRow = 0,
                            const unsigned long  theColumn = 0 );
    //  Returns a pointer to storage containing the attribute value of
    //  the field with the given tuple and field index.
    //  The data is a character string representation (e.g. ASCII)
    //  unless DataIsBinary() returns true.
    //  NB. the returned memory location should not be written to
    //  and should only be assumed to be accessable until this
    //  NsqliteResult object is either destroyed or re-used.
    //  Parameters:
    //    0-based coordinates of attribute to get.


    bool isDataNull(    const unsigned long  theRow = 0,
                        const unsigned long  theColumn = 0 );
    //  Returns true if the given attribute has a NULL value.
    //  Parameters:
    //    0-based coordinates of attribute to test.

    unsigned long  getDataLength(   const unsigned long  theRow = 0,
                                    const unsigned long  theColumn = 0 );
    //  Returns the length in bytes of the given attribute.
    //  Parameters:
    //    0-based coordinates of attribute to test.

    unsigned long getRowsAffectedCount();
    // Returns the number of rows (tuples) affected by the query that
    // resulted in this result set.

    void dumpResult();
    // Aid to debugging and development.
    // Just prints out the status of the result object.

    void clear();
    // Emptry the result object. ( Free up memory used to store results ).

    // Following 3 methods should just be called by the Nsqlite object.
    // Until I think of a better way, these have to be public so nsqlite can see them.

    void setErrorMessage( const char* theErrorMessage );

    void setRowsAffectedCount( unsigned long theCount );

    static int callback(    void* theUserParam,
                            int argc,
                            char** argv,
                            char** azColName );
private:
    typedef struct
        {
        std::string value;
        bool        isNull;
        } NSQLITERESULT_DB_FIELD;

    typedef std::vector< NSQLITERESULT_DB_FIELD > NSQLITERESULT_DB_ROW;

    std::vector< std::string >          m_columnNames;
    std::vector< NSQLITERESULT_DB_ROW > m_data;
    std::string                         m_errorMessage;
    unsigned long                       m_rowsAffectedCount;
};

#endif
