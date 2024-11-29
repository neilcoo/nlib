// ncrc.cxx by Neil Cooper. See ncrc.h for documentation
#include "ncrc.h"

#include <stdio.h>  // for FILE

static void makeCrcTable( unsigned long* theTable );


unsigned long computeMemoryCrc32(   const unsigned char* theBuffer,
                                    const size_t         theBufferLength,
                                    const unsigned long  thePreviousCrc32 )
{
    static unsigned long crcTable[ 256 ];
    static bool tableBuilt = false;

    unsigned long crc32 = thePreviousCrc32 ^ 0xFFFFFFFF;
    size_t i;

    if ( !tableBuilt )
        {
        makeCrcTable( crcTable );
        tableBuilt = true;
        }

    for ( i = 0; i < theBufferLength; i++ )
        crc32 = ( crc32 >> 8 ) ^ crcTable[ ( crc32 ^ theBuffer[i] ) & 0xFF ];
    return( crc32 ^ 0xFFFFFFFF );
}


bool computeFileCrc32(  FILE* theFile,
                        unsigned long* theCrc32,
                        unsigned long  theChunkSize )
{
    unsigned char* chunk = new unsigned char[ theChunkSize ];
    bool status = true;
    size_t readLength = 0;
    unsigned long runningTotal = 0;

    do
        {
        readLength = fread( chunk, 1, theChunkSize, theFile );
        if ( readLength )
            runningTotal = computeMemoryCrc32( chunk, readLength, runningTotal );
        else
            status = ( !ferror( theFile ) );
        }
    while ( status && readLength );

    if ( status )
    *theCrc32 = runningTotal;

    delete[] chunk;

    return( status );
}


void computeCrc(  const Nbinary& data,
                  const Nbinary& poly,
                  const Nbinary& initial,
                  Nbinary&       result   )
{
    const unsigned int nbits = poly.length();
    char* crc = new char[ nbits - 1 ];
    result.initialise( nbits - 1 );
    unsigned int i = 0;

    for ( i = 0; i < ( nbits-1 ); i++ )
        crc[ i ] = 0;

    for ( i = 0; i < ( nbits-1 ); i++ )
        crc[ i ] = initial[ ( nbits - 2 ) - i ] == '1' ? 1 : 0;

    for ( i = 0; i < data.length(); i++ )
        {
        char doInvert = ( data[ i ] == '1' ) ^ crc[ nbits - 2 ]; // XOR required?

        for( int j = nbits - 2; j > 0; j-- )
            if ( poly[ ( nbits - j ) - 1 ] == '1' )
                crc[ j ] = crc[ j - 1 ] ^ doInvert;
            else
                crc[ j ] = crc[ j - 1 ];
        crc[ 0 ] = doInvert;
        }

    for ( i = 0; i < ( nbits - 1 ); i++ )
    result[ ( nbits - 2 ) - i ] = crc[ i ] ? '1' : '0';   // Convert binary to ASCII

   delete[] crc;
}


// Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
// x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.
//
// Polynomials over GF(2) are represented in binary, one bit per coefficient,
// with the lowest powers in the most significant bit.  Then adding polynomials
// is just exclusive-or, and multiplying a polynomial by x is a right shift by
// one.  If we call the above polynomial p, and represent a byte as the
// polynomial q, also with the lowest power in the most significant bit (so the
// byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
// where a mod b means the remainder after dividing a by b.
//
// This calculation is done using the shift-register method of multiplying and
// taking the remainder.  The register is initialized to zero, and for each
// incoming bit, x^32 is added mod p to the register if the bit is a one (where
// x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
// x (which is shifting right by one and adding x^32 mod p if the bit shifted
// out is a one).  We start with the highest power (least significant bit) of
// q and repeat for all eight bits of q.
//
// The table is simply the CRC of all possible eight bit values.  This is all
// the information needed to generate CRC's on data a byte at a time for all
// combinations of CRC register values and incoming bytes.

static void makeCrcTable( unsigned long* theTable )
{
    // Terms of polynomial defining this crc (except x^32):
    static const unsigned char p[] = { 0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26 };

    unsigned long poly;  // Polynomial exclusive-or pattern
    unsigned long c;
    unsigned int n, k;

    // Make exclusive-or pattern from polynomial ( 0xedb88320 )
    poly = 0;
    for ( n = 0; n < sizeof( p ) / sizeof( unsigned char ); n++ )
        poly |= ( unsigned long )1 << ( 31 - p[n] );

    for ( n = 0; n < 256; n++ )
        {
        c = n;
        for ( k = 0; k < 8; k++ )
            c =  ( c & 1 ) ? ( poly ^ ( c >> 1 ) ) : ( c >> 1 );
        theTable[ n ] = c;
        }
}

