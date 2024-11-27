#include <iostream>
#include "nerror.h"
#include "npsql.h"

using namespace std;

int main(int ac, char* av[])
{
    HANDLE_NERRORS;

    if (ac != 3)
        ERROR ( "Usage: query <dbname> \"query\"" );

    Npsql db( av[1] );
    NpsqlResult result;

    if ( db.query( av[2], &result, true ) )
        cout << "success" << endl;
    else
        cout << "fail" << endl;

    int rows = result.getRowCount();
    int cols = result.getColumnCount();

    cout << rows << " rows, " << cols << " columns returned." << endl;
    for (int y = 0; y < rows; y++)
            for ( int x = 0; x < cols; x++ )
                {
                if ( ( x== 0) && (y == 0) )
                    for ( int c = 0; c < cols; c++ )
                        {
                        cout << "'" << result.getColumnNameFromIndex(c) << "'(length " << result.getColumnLength(c) << ")";
                        if ( c == (cols-1) )
                            cout << endl;
                        else
                            cout << " , ";
                        }

                if ( result.isDataNull( y, x ) )
                    cout << "<NULL>";
                else
                    cout << "'" << result.getData(y,x) <<"'(length " << result.getDataLength(y,x) << ") ";
                if ( x == (cols-1) )
                    cout << endl;
                else
                    cout << " , ";
                }

    cout << "Command status = '" << result.getCommandStatus() << "'" << endl;
    cout << result.getRowsAffectedCount() << " rows affected." << endl;
}

