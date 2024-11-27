#include <stdio.h>

#include "ncrc.h"


int main( int argc, const char *argv[] )
{
   if ( argc < 2 )
      {
      unsigned long crc32;
      //read from 'stdin' if no arguments given
      if ( computeFileCrc32( stdin, &crc32 ) )
         printf("%08lX\n", crc32 );
      else
         fprintf( stderr, "%s: error reading from stdin", argv[0] );
      }
   else
      {
      int argIdx;

      for ( argIdx = 1; argIdx < argc; argIdx++ )
         {
         const char* filename = argv[ argIdx ];
         unsigned long crc32 = 0;
         FILE* file = fopen( filename, "rb" );
         if ( !file )
            fprintf( stderr, "error opening file '%s'\n", filename );
         else
            {
            if ( computeFileCrc32( file, &crc32 ) )
               printf("%08lX\n", crc32 );
            else
               fprintf( stderr, "error reading from file '%s'\n", filename );
            fclose( file );
            }
         }
      }
   return( 0 );
}

