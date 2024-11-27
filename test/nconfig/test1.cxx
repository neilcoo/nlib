#include <iostream>
#include "nerror.h"
#include "nconfig.h"

using namespace std;

int Safemain(int ac, char* av[] )
{
   Nconfig config( "test.cfg", false );

 cout << "erase key2 returned " << config.eraseValue("key2") << endl;
// cout << "erase key4 returned " << config.EraseValue("key4") << endl;

   cout << "opened OK " << endl;
   string in;
   while( true )
      {
      cin >> in;
      cout << "finding '" << in << "' found ";
      string found;
      if ( config.getValue( in, found ) )
         cout << "'" << found << "'" << endl;
      else
         cout << "nothing!" << endl;
      }
}

int main(int ac, char* av[] )
{
   NERROR_HANDLER( Safemain( ac, av) );
}
