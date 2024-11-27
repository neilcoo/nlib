#include <iostream>
#include "nerror.h"

#include "nxml.h"


int main(int ac, char* av[])
{
    HANDLE_NERRORS;

    Nxml myXmlfile("test.xml", "test.xsd", false );
//    Nxml myXmlfile("t2.xml" );

    LOG("XML loaded OK");

    myXmlfile.dumpTree();

    return EXIT_SUCCESS;
}
