// nrandom.cxx by Neil Cooper. See nrandom.h for documentation
#include "nrandom.h"

#include <stdlib.h>

Nrandom::Nrandom( unsigned int seed ) : m_seed( seed )
{
}


Nrandom::~Nrandom()
{
}


float Nrandom::getRandomNumber( float minimum, float maximum )
{
    float rnum = rand_r( &m_seed );
    if ( rnum == 0 )   // avoid divide by 0
        rnum = minimum;
    else
        rnum = ( ( rnum/(float)RAND_MAX ) * ( maximum - minimum ) ) + minimum;

    return rnum;
}
