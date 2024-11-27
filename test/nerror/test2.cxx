#include <iostream>
#include "nerror.h"


void SafeMain(int ac, char* av[])   // Use this like 'real' main ....
{
    ERROR( "Some error hit" );
    std::cout << "this message should never appear" << std::endl;
}


int main(int ac, char* av[])
{
   std::cout << "Starting up" << std::endl;

   NERROR_HANDLER( SafeMain( ac, av ) );
}
