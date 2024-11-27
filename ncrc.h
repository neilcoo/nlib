#ifndef NCRC_H
#define NCRC_H

// Ncrc v1.2 by Neil Cooper 31st October 2017
// Implements easy-to-use CRC helper functions.

#include "nbinary.h"

unsigned long computeMemoryCrc32(   const unsigned char* theBuffer,
                                    const size_t         theBufferLength,
                                    const unsigned long  thePreviousCrc32 = 0 );
    // Compute CRC-32 value for a block of memory.
    // Parameters:
    //    theBuffer: Address of data to calculate crc-32 for.
    //    theBufferLength: Length of data to calculate crc-32 for.
    //    thePreviousCrc32: Allows for a single CRC-32 to be calculated from a
    //    series of calls to ComputeMemoryCrc32.
    // Return:
    //    Calculated CRC-32.


bool computeFileCrc32(  FILE*                theFile,
                        unsigned long*       theCrc32,
                        const unsigned long  theChunkSize = 8192 );
    // Compute CRC-32 value for a file.
    // Parameters:
    //    theFile: File handle of open file to calculate crc-32 for.
    //             File pointer must already be at beginning of file.
    //    theCrc32:   Pointer to variable to hold calculated CRC-32 value.
    //    theChunkSize: Size of chunk to use when reading file.
    // Return:
    //    true on success, false on file error.


void computeCrc(    const Nbinary& data,
                    const Nbinary& poly,
                    const Nbinary& initial,
                    Nbinary&       result   );
    // Compute CRC using given polynomial and starting value, for given data.
    // Note: The length in bits of the resulting CRC will be the length - 1 of
    // the given polynomial, e.g: for an 8 bit CRC the poly must be 9 bits long.
    // Parameters:
    //    data: Data to calculate CRC for.
    //    poly: Polynomial to use. Note: The value of the MSB (leftmost bit) is ignored
    //                                   but conventionally is always 1.
    //    initial: starting CRC value.
    //    result:  Returned CRC.

// Useful note to remember: If the data you perform a CRC on already has a previously
// computed and valid CRC appended to it, the resultant CRC will always be 0.

#endif

