#ifndef NARGS_H
#define NARGS_H

// Nargs v1.2 by Neil Cooper 5th June 2018
// Implements easy-to-use argv processing

// Example:
//    Consider the command line: myapp -v -f outfile infile
//    It has one argument (infile), 2 options (-v and -f) and one option parameter (outfile, dependent on/attached to -f)
//    Note: the following form will also be parsed correctly and is logically identical to the above: myapp -vfoutfile infile
//
// Code Example for processing returned parameters and options:
//
//   Nargs::ARGUMENTS rp = args.GetArguments(); // Returns arguments
//   for ( int i=0; i <  rp.size(); i++ )
//      cout << "Argument " << i << " = '" << rp[i] << "'" << endl;
//
//   Nargs::OPTIONS op = args.GetOptions();     // Returns options
//   for ( Nargs::OPTIONS::iterator it=op.begin(); it!=op.end(); ++it)
//      {
//      cout << "Option " << it->first << endl;
//      for ( int j=0; j <  it->second.size(); j++ )
//         cout << "     Option Param " << j << " = '" << it->second[j] << "'" << endl;
//      }

#include <string>
#include <map>
#include <vector>

class Nargs
{
public:
    typedef std::string                       ARGUMENT;
    typedef std::vector<ARGUMENT>             ARGUMENTS;
    typedef std::string                       OPTION;
    typedef std::vector<std::string>          OPTION_PARAMS;
    typedef std::map<OPTION, OPTION_PARAMS>   OPTIONS;

    Nargs(   const int            argc,             // As passed into main()
            const char* const    argv[],           // As passed into main()
            const std::string    allowedOptions = "-help -version",  // eg. "af1 -help v" means options --help, -a, -f and -v are allowed.
                                                                     // --help, -a and -v require no parameters, f requires 1 parameter
            const unsigned short minArgCount = 0,  // Minumum number of arguments allowed
            const short          maxArgCount = -1  // Maximum number of arguments allowed ( < 0=don't check, < minArgCount=minArgCount )
         );


    virtual ~Nargs();

    bool argsAreValid(); // returns true only if given argc/argv complies with rules given to constructor

    std::size_t    getArgumentCount();  // Returns count of arguments given (not including argv[0]). Example above would return 1.

    std::size_t    getOptionCount();    // Returns count of options given. Example above would return 2

    std::string    getCommand();        // Returns argv[0]

    bool           optionIsSpecified( const OPTION theOption ); // returns true when given option was specified on command line

    ARGUMENTS      getArguments();     // Returns regular params

    OPTIONS        getOptions();        // Returns options

    std::string    getParseErrorMessage(); // Returns human-readable error message if argsAreValid() returns false;

private:
    std::string                      m_argv0;
    ARGUMENTS                        m_args;
    OPTIONS                          m_options;
    std::string                      m_parseError;
    std::map<OPTION, unsigned int>   m_allowed;

    void parseArgv( const int argc, const char* const argv[] );
    void loadAllowables( const std::string& allowedOptions );
    std::string parseAllowableString( std::string& token );
    bool optionIsAllowed( OPTION theOption, unsigned int* nParams = NULL );
    unsigned int addOptionsIfValid( OPTION theOption, const bool isFullWordOption );
    void addOptionIfNotDuplicate( OPTION& theOption, OPTION_PARAMS& theParams );
    bool lTrim( std::string& theString, const std::string theWhitespaces = " " );
    bool rTrim( std::string& theString, const std::string theWhitespaces = " " );
};

#endif
