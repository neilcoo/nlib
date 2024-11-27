#include <iostream>
#include <sstream> // for ostringstream
#include "nerror.h"
#include "nsqlite.h"

using namespace std;

int main( int ac, char*av[] )
{
    HANDLE_NERRORS;

    if ( ac != 3 )
        ERROR( "Usage: ", av[0], " <dbname> <sql-query>" );

    Nsqlite db( av[1] );

    NsqliteResult result;

    if ( !db.query( av[2], &result ) )
        ERROR( "Query failed: ", result.getErrorMessage() );

    result.dumpResult();

    int columnCount = result.getColumnCount();
    int rowCount = result.getRowCount();

    for (int i=0; i < columnCount; i++ )
        {
        cout << result.getColumnNameFromIndex( i );
        if ( i < ( columnCount -1 ) )
            cout << " \t";
        }
    cout << endl;

    for (int i=0; i < rowCount; i++ )
        {
        for (int j=0; j < columnCount; j++ )
            {
            if ( result.isDataNull(i,j) )
                cout << "NULL";
            else
            cout << "'" << result.getData( i, j) << "'";

            if ( j < ( columnCount -1 ) )
                cout << " \t";
            }
        cout << endl;
        }

    return EXIT_SUCCESS;
}

