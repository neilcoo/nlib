#ifndef NRANDOM_H
#define NRANDOM_H

// Nrandom v1.0 by Neil Cooper 9th Dec 2019
// Implements threadsafe random number generator.

#include "ntime.h"

class Nrandom
{
public:
    Nrandom( unsigned int seed = time( 0 ) );

    ~Nrandom();

    float getRandomNumber( float minimum = 0, float maximum = (float)RAND_MAX );

protected:
    unsigned int m_seed;
};


#endif



