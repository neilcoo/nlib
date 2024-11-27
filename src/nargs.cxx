// nargs.cxx by Neil Cooper. See nargs.h for documentation
#include "nargs.h"

#include <nerror.h>
#include <string.h> // for strlen()

#include "ntokeniser.h"


using namespace std;

static const char OPTION_CHAR = '-';

// static const string OPT_TOKEN_TERMINATORS = " \t\f\v\n\r";


bool stringToInt( string token, int* n = NULL )
// Convert (possibly signed) string to int.
// If the string parameter doesn't start with an integer, return value is false and *n is unmodified.
// If the string parameter contains or starts with an int, return value is true, and *n contains the converted value.
//
{
    int val = 0;

    bool converted = true;
    try
        {
        val = std::stoi( token );
        if ( n )
            *n = val;
        }
    catch( invalid_argument& )
        {
        converted = false;
        }
    return converted;
}


/*
void dumpallowables()
{
   for ( std::map<OPTION, unsigned int>::iterator it = m_allowed.begin(); it != m_allowed.end(); it++ )
      std::cout << "'" << it->first << "' => " << it->second << '\n';
}
*/


Nargs::Nargs(  const int            argc,          // As passed into main()
               const char* const    argv[],        // As passed into main()
               const string         allowedOptions,// eg. "vf 1" means -v and -f can be specified, v never has params, f must have 1 following
               const unsigned short minArgCount,   // Minumum number of arguments required
               const short          maxArgCount    // Maximum number of arguments allowed ( < 0=don't check, < minArgCount=minArgCount )
            )
{
    if ( argc < 1 )
        ERROR(  "Nargs: argc passed in with illegal value: ", argc, " (i.e. less than 1)." );

    short maxArgs = maxArgCount;

    if ( ( maxArgCount < minArgCount ) &&  ( maxArgCount >= 0 ) )
        maxArgs = minArgCount;

    m_argv0 = argv[0];                  // save argv[0]

    loadAllowables( allowedOptions );   // Parse and store allowed options list
    // dumpallowables();

    parseArgv( argc, argv );         // Parse command line per allowed options list

    // Check we have the right number of arguments
    if ( argsAreValid() )
        {
        if ( ( m_args.size() < minArgCount ) || ( ( (int)m_args.size() > maxArgs ) && ( maxArgs >= 0 ) ) )
            {
            ostringstream msg;
            msg << m_args.size() << " argument";
            if ( m_args.size() != 1 )
                msg << "s";
            msg << " given, ";
            if ( ( minArgCount < maxArgs ) && ( maxArgs >= 0 ) )
                msg << "from " << minArgCount << " to " << maxArgs;
            else
                msg << minArgCount;
            msg << " required.";

            m_parseError = msg.str();
            }
    }
}


Nargs::~Nargs()
{
}


string Nargs::parseAllowableString( string& token ) //called for single tokens that look like vf1d12c
// meaning v option with 0 params, f option must have 1 param,  d option must have 12 params
// c option possibly 0 or with later specified number of params
// If an option exists that possibly has a later specified number of params, return it. If not return an empty string.
{
    string param;
    if ( stringToInt( token ) )
        ERROR( "Nargs:: Allowed arguments syntax error: '", token, "' begins with a number" );

    while ( token.length() )
        {
        int n;
        param = token[0];
        // consume the option
        token = token.substr( 1 );
        if ( param[0] == OPTION_CHAR )
            {
            param = token;  // just consider the whole rest of the token to be the param
            token.clear();  // clear token so we exit with param as the token
            }
        else  // param is not OPTION_CHAR so param is a single char option
        if ( token.length() )
            {
            if ( stringToInt( token, &n ) )       // If a number exists (i.e. number of params required)...
                {
                // add the option immediately as we know everything about it
                m_allowed[ param ] = n;
                param.clear(); // clear it so we don't re-store it later
                // consume the numeric value
                do
                    {
                    if ( token.length() )
                        token = token.substr( 1 );
                    } while ( token.length() && stringToInt( token, &n ) );
                }
            else  // More follows that isn't a number, store this option with a default of zero required
                {
                m_allowed[ param ] = 0;
                param.clear();  // clear it so we don't re-store it later
                }
            }
        }

   return param;
}


void Nargs::loadAllowables( const string& allowedOptions )
{
    string opts = allowedOptions;
    string token = Ntokeniser::consumeToken( opts );
    string lastOpt;

    while ( !token.empty() )
        {
        int nParamsReqd = 0;
        bool tokenStartsWithCount = stringToInt( token, &nParamsReqd );

        if ( tokenStartsWithCount )
            {
            if ( lastOpt.length() )
            {
            m_allowed[ lastOpt ] = nParamsReqd;
            lastOpt.clear();
            // now remove count from front and parse rest of token by going round loop again
            while ( token.length() && stringToInt( token ) )
            token = token.substr(1);
            }
            else
            ERROR( "Nargs:: Allowed arguments syntax error: count parameter: '", token, "' provided before argument" );
            }
        else // token is not count
            {
            int tokLen = token.length();
            if ( tokLen && lastOpt.length() ) // tokLen means more to process: lastOpt will get overwritten so make it allowed here
                {
                m_allowed[ lastOpt ] = 0;
                lastOpt.clear();
                }
            if ( tokLen == 1 )
                if ( token[0] == OPTION_CHAR )
                    ERROR( "Nargs:: Allowed arguments syntax error: '", OPTION_CHAR, "' alone is not an allowable argument" );
                else
                lastOpt = token;  // it was a single letter option and not an OPTION_CHAR

            else // token length > 1
                if ( token[0] == OPTION_CHAR )
                    lastOpt = token.substr( 1 ); // Its a whole word option, save the whole thing
                else
                    lastOpt = parseAllowableString( token );
            token.clear(); // we've handled it all by here
            }
        if ( !token.length() ) //necessary because we can only parse token partially first time round loop if tokenStartsWithCount
            token = Ntokeniser::consumeToken( opts );
        }

    if ( lastOpt.length() )
        m_allowed[ lastOpt ] = 0;
}



bool Nargs::argsAreValid()
// returns true only if given argc/argv complies with rules given to constructor
{
    return ( !m_parseError.length() );
}


size_t Nargs::getArgumentCount()
// Returns count of regular params given (not including argv[0]). Example in .h file would return 1
{
    return m_args.size();
}


size_t Nargs::getOptionCount()
// Returns count of options given. Example in .h file would return 2
{
    return m_options.size();
}


string Nargs::getCommand()
// Returns argv[0]
{
    return m_argv0;
}


bool Nargs::optionIsSpecified( const OPTION theOption )
// returns true only if given option was specified on command line
{
    return ( m_options.find( theOption ) != m_options.end() );
}


Nargs::OPTIONS Nargs::getOptions()
// Returns options
{
    return m_options;
}

Nargs::ARGUMENTS Nargs::getArguments()
{
    return m_args;
}


string Nargs::getParseErrorMessage()
// Returns human-readable error message if ArgsAreValid() returns false
{
    return m_parseError;
}


void Nargs::parseArgv( const int argc, const char* const argv[] )    // Parse and store argv
{
    unsigned int noOfOptionParamsOutstanding = 0;
    OPTION option;
    OPTION lastOption;

    for ( unsigned int i=1; ( i < (unsigned int)argc ) && argsAreValid(); i++ ) // Parse each arg[] (1 = skip argv[0])
        {
        if ( argv[i][0] != OPTION_CHAR ) // Its an argument or option param
            {
            if ( noOfOptionParamsOutstanding ) // Its an option param
                {
                noOfOptionParamsOutstanding--;
                m_options[option].push_back( argv[i] );
                }
            else // its an argument
                m_args.push_back( argv[i] );
            }
        else  // its one or more options
            {
            lastOption = option;
            option = argv[ i ];
            option = option.substr(1); // remove the leading OPTION_CHAR
            bool fullWordOption = ( option.length() && ( option[0] == OPTION_CHAR ) );

            if ( fullWordOption )
                option = option.substr(1); // remove the second OPTION_CHAR

            if ( noOfOptionParamsOutstanding )
                {
                ostringstream msg;
                msg << "Option '" << lastOption <<"' requires " << noOfOptionParamsOutstanding << " more parameters.";
                m_parseError = msg.str();
                }
            else
                {
                if ( !option.length() )
                    {
                    ostringstream msg;
                    msg << "Parameter " << i <<" is an option flag with no actual option following.";
                    m_parseError = msg.str();
                    }
                else
                noOfOptionParamsOutstanding = addOptionsIfValid( option, fullWordOption );
                }
            }
        }

    if ( argsAreValid() && noOfOptionParamsOutstanding )
        {
        ostringstream msg;
        msg << "Option '" << option <<"' requires " << noOfOptionParamsOutstanding << " more parameter";
        if ( noOfOptionParamsOutstanding > 1 )
            msg << "s.";
        else
            msg << ".";
        m_parseError = msg.str();
        }
}


void Nargs::addOptionIfNotDuplicate( OPTION& theOption, OPTION_PARAMS& theParams )
{
    if ( optionIsSpecified( theOption ) )  // If we've already seen this option
        {
        ostringstream msg;
        msg << "Option '" << theOption <<"' specified more than once.";
        m_parseError = msg.str();
        }
    else
        m_options[ theOption ] = theParams;
}


unsigned int Nargs::addOptionsIfValid( OPTION theOption, const bool isFullWordOption )
// returns number of option parameters outstanding, removes
{
    unsigned int optParamsLeft = 0;

    if ( isFullWordOption )
        {
        if ( optionIsAllowed( theOption, &optParamsLeft ) )   // If option exists in the allowed string
            {
            OPTION_PARAMS op;
            addOptionIfNotDuplicate( theOption, op );          // just add the whole option
            }
        }
    else     // Its one or more single char options
        while( theOption.length() )
            {
            OPTION opt;
            opt = theOption[0];
            theOption = theOption.substr( 1 );                 // consume first char
            if ( optionIsAllowed( opt, &optParamsLeft ) )
                {
                OPTION_PARAMS optParams;
                if ( optParamsLeft && theOption.length() )      // If the option needs parameters and anything follows
                    {
                    optParams.push_back( theOption );            // Use the rest of the option as an option param
                    theOption.clear();                           // Consume it all
                    optParamsLeft--;
                    }
                addOptionIfNotDuplicate( opt, optParams );
                }
            }
    return optParamsLeft;
}


bool Nargs::optionIsAllowed( OPTION theOption, unsigned int* nParams )
// if given option appears in allowed list, set nparams (if provided) to
// the number of params the option requires, and return true
{
    bool found = ( m_allowed.find( theOption ) != m_allowed.end() );
    if ( found )
        {
        if ( nParams )
        *nParams=m_allowed[ theOption ];
        }
    else
        {
        ostringstream msg;
        msg << "Option '" << theOption <<"' not recognised.";
        m_parseError = msg.str();
        }

    return found;
}


bool Nargs::lTrim( string& theString, const string theWhitespaces )
{
    unsigned long offset = theString.find_first_not_of( theWhitespaces );
    unsigned long length = theString.length();

    if ( offset == string::npos )	// whole string is trim characters or empty
        theString.erase();
    else
        if (offset)
            theString.erase( 0, offset );

    return( ( offset != 0 ) && length );
}


bool Nargs::rTrim( string& theString, const string theWhitespaces )
{
    unsigned long offset = theString.find_last_not_of( theWhitespaces );
    unsigned long length = theString.length();

    if ( offset == string::npos )	// whole string is trim characters or empty
        theString.erase();
    else
        {
        offset++;
        if ( offset != length )
            theString.erase( offset );
        }
    return( ( offset != length ) && ( length > 0 ) );
}

