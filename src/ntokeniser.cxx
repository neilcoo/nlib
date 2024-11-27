// ntokeniser.cxx by Neil Cooper. See ntokeniser.h for documentation
#include "ntokeniser.h"

#include <string>

using namespace std;

const std::string Ntokeniser::DEFAULT_TOKEN_TERMINATORS( " ,\t\f\v\n\r" );


string Ntokeniser::consumeToken( string& theTokenStream, const string& theTerminators )
{
    string token;

    size_t tokStart = theTokenStream.find_first_not_of( theTerminators );
    if ( tokStart == string::npos )
        theTokenStream.clear();
    else
        {
        size_t firstTerm = theTokenStream.find_first_of( theTerminators, tokStart );
        if ( firstTerm == string::npos )
            {
            token = theTokenStream.substr( tokStart );
            theTokenStream.clear();
            }
        else
            {
            token = theTokenStream.substr( tokStart, firstTerm - tokStart );
            // consume token and (only) its terminator, since subsequent calls may use different terminators.
            theTokenStream = theTokenStream.substr( firstTerm + 1, theTokenStream.length() - ( firstTerm + 1 ) );
            }
        }

   return token;
}
