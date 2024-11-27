#include <iostream>
#include "nerror.h"
#include "nthreadPool.h"
#include "unistd.h"

using namespace std;


void Threadproc(void* theParam)
{
    while(1);
}


int main2(int ac, char* av[] )
{
    string s;
    while(1)
        {
        getline(std::cin, s);
        cout << "'" << s << "'" << endl;
        }

    return (0);
}


int main(int ac, char* av[] )
{
    HANDLE_NERRORS;

    const int NUMBER_OF_THREADS = 12;

    NthreadPool* pool = new NthreadPool( NUMBER_OF_THREADS, "bob" );

    for ( int i = 0; i < NUMBER_OF_THREADS; i++ )
        pool->submitJob( Threadproc );

    unsigned long long coreFlags = 1;
    while(1)
        {
        string s;
        getline( cin, s );
        cout << coreFlags << endl;
        pool->updatePoolAffinity( coreFlags );
        coreFlags = coreFlags * 2;
        }
    cout << "main done" << endl;
    return (0);
}

