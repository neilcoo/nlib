#include <iostream>

#include "nerror.h"


class myException : public std::exception
{
    const char* what() const noexcept
    {
        return "GeeksforGeeks!!";
    }
};

int main(int ac, char* av[])
{
    HANDLE_NERRORS;

    LOG ( "Starting up" );
    WARN("About to cause an error with code ", 42 );

    ERROR( "Guru meditation: ", 42 );   // NerrorException test
    // throw myException();                 // std::exception test

   std::cout << "this message should never appear" << std::endl;
}



