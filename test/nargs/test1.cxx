#include <iostream>
#include "nerror.h"
#include "nargs.h"

using namespace std;

void Safemain(const int ac, const char* const av[] )
{
//   Nargs args( ac, av, 1,1, "ab -help 0 cf 1 " );
//   Nargs args( ac, av, 1,1, "-help 1" );
//   Nargs args( ac, av, 1,1, "-help" );
// Nargs args( ac, av, 1, 1, "ab x1y01z 1-help 1" );
 // Nargs args( ac, av, " z 1-wow 1 -help -version", 0, 1 );

    Nargs args( ac, av, "-help r", 0, 0 );

//   Nargs args( ac, av );


   bool valid = args.argsAreValid();
   cout << "ArgsAreValid() returned " << ( valid ? "true" : "false" ) << endl; // returns true only if given argc/argv complies with rules given to constructor

   cout << "getParseErrorMessage() returned '" << args.getParseErrorMessage() << "'" << endl;

   cout << "getArgumentCount() returned " << args.getArgumentCount() << endl; // Returns count of regular params given (not including argv[0]).

   cout << "getOptionCount() returned " << args.getOptionCount() << endl;  // Returns count of options given.

   cout << "getCommand() returned " << args.getCommand() << endl; // Returns argv[0]

   cout << "optionIsSpecified(\"b\") returned " << args.optionIsSpecified( "b" ) << endl; // returns true only if given option was specified on command line

   cout << "optionIsSpecified(\"help\") returned " << args.optionIsSpecified( "help" ) << endl;

   cout << "optionIsSpecified(\"version\") returned " << args.optionIsSpecified( "version" ) << endl;

   Nargs::ARGUMENTS rp = args.getArguments();  // Returns arguments

   Nargs::OPTIONS op = args.getOptions();  // Returns options

   for ( int i=0; i <  rp.size(); i++ )
      cout << "Regular Param " << i << " = '" << rp[i] << "'" << endl;

   for ( Nargs::OPTIONS::iterator it=op.begin(); it!=op.end(); ++it)
      {
      cout << "Option " << it->first << endl;
      for ( int j=0; j <  it->second.size(); j++ )
         cout << "     Option Param " << j << " = '" << it->second[j] << "'" << endl;
      }
}


int main(const int ac, const char* const av[]  )
{
   NERROR_HANDLER( Safemain( ac, av) );
}
