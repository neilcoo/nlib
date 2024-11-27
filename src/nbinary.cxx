// nbinary.cxx by Neil Cooper. See nbinary.h for documentation
#include "nbinary.h"
#include <string.h> // for ostringstream

// Neil TODO:: reimplement using std::stoull?

#include <cmath>    // for log2

#include "nerror.h"

using namespace std;


Nbinary::Nbinary()
{
}


Nbinary::Nbinary( const Nbinary& other )
{
    m_value = other.m_value;
}


Nbinary::Nbinary( const string& value )
{
    setByString( value );
}


Nbinary::Nbinary( const char* value )
{
    setByString( value );
}


Nbinary::Nbinary( const unsigned long long intValue, const size_t lengthInbits, const bool allowTruncate )
{
    setByInt( intValue, lengthInbits, allowTruncate );
}


Nbinary::~Nbinary()
{
}


Nbinary& Nbinary::operator = ( const string& value )
{
    setByString( value );
    return *this;
}


Nbinary& Nbinary::operator = ( const unsigned long long value )
{
    setByInt( value );
    return *this;
}


bool Nbinary::operator == ( const Nbinary& other ) const
{
    return ( m_value == other.m_value );
}


const char Nbinary::operator[] ( const int index ) const
{
    return m_value[ index ];
}


char& Nbinary::operator[] ( const int index )
{
    return m_value[ index ];
}


bool Nbinary::initialise( const unsigned long long value, const size_t lengthInbits, const bool allowTruncate )
{
    return setByInt( value, lengthInbits, allowTruncate );
}


bool Nbinary::setBitLength( const size_t lengthInbits, const bool allowTruncate )
{
    bool valueRetained = true;
    int change = lengthInbits - m_value.length();
    string tempValue = m_value;

    while ( change < 0 && tempValue.length() )
        {
        if ( tempValue[ 0 ] == '1' )
            valueRetained = false;
        tempValue = tempValue.substr( 1, tempValue.length() - 1 );
        change++;
        }

    if ( change > 0 )
        tempValue = string( change, '0' ) + tempValue;

    if ( !valueRetained && !allowTruncate )
        {
        ERROR( "Given value cannot be stored in requested number of bits" );
        }
    else
        m_value = tempValue;

    return valueRetained;
}


size_t Nbinary::length() const
{
    return m_value.length();
}


string Nbinary::getAsBinary() const
{
    return m_value;
}


string Nbinary::getAsHex() const
{
    ostringstream ostr;
    ostr << hex << uppercase << getAsInt();
    return ostr.str();
}


unsigned long long Nbinary::getAsInt() const
{
   unsigned long long b = 1;
   unsigned long long r = 0;

   for ( int i = m_value.size() - 1; i >= 0; i-- )
      {
      if ( m_value[i] == '1' )
         r = r + b;
      b = b * 2;
      }

   return r;
}


void Nbinary::setByString( const string& value )
{
   if ( ( value.substr( 0, 2 )  == "0x" ) || ( value.substr( 0, 2 )  == "0X" ) )
      setByHex( value.substr( 2, value.length() - 2 ) );
   else
      if ( ( value.substr( 0, 2 )  == "0b" ) || ( value.substr( 0, 2 )  == "0B" ) )
         setByBinary( value.substr( 2, value.length() - 2 ) );
      else
         setByInt( stoull( value, NULL, 0 ) );
}


void Nbinary::setByHex( const string& inHex )
{
   string in( inHex );
   m_value.clear();

   while ( in.size() )
      {
      switch( in[0] )
         {
         case '0':
            m_value += "0000";
            break;

         case '1':
            m_value += "0001";
            break;

         case '2':
            m_value += "0010";
            break;

         case '3':
            m_value += "0011";
            break;

         case '4':
            m_value += "0100";
            break;

         case '5':
            m_value += "0101";
            break;

         case '6':
            m_value += "0110";
            break;

         case '7':
            m_value += "0111";
            break;

         case '8':
            m_value += "1000";
            break;

         case '9':
            m_value += "1001";
            break;

         case 'A':
         case 'a':
            m_value += "1010";
            break;

         case 'B':
         case 'b':
            m_value += "1011";
            break;

         case 'C':
         case 'c':
            m_value += "1100";
            break;

         case 'D':
         case 'd':
            m_value += "1101";
            break;

         case 'E':
         case 'e':
            m_value += "1110";
            break;

         case 'F':
         case 'f':
            m_value += "1111";
            break;

         case ' ':
            break;

         default:
            ERROR( "Input string contains non-hex character '", inHex[0], "' (ignored)" );
         }
      in.erase( 0, 1 );
      }
}


void Nbinary::setByBinary( const string& inBinary )
{
   string tempValue;
   for ( size_t i=0; i < inBinary.length(); i++ )
      {
      unsigned char c = inBinary[i];

      switch( c )
         {
         case '0':
            tempValue += "0";
            break;

         case '1':
            tempValue += "1";
            break;

         case ' ':
            break;

         default:
            ERROR( "Input string contains non-binary character '", c, "'" );
         }
      }
   m_value = tempValue;
}


bool Nbinary::setByInt( const unsigned long long intValue, const size_t lengthInbits, const bool allowTruncate )
{
    unsigned long long intVal = intValue;
    size_t bitLen = lengthInbits ? lengthInbits : getMinBitLength( intValue );
    string stringVal;

    stringVal.resize( bitLen, '0' );

    for ( int i = bitLen - 1; i >= 0; i-- )
        {
        ( intVal & 1 ) ? stringVal[ i ] = '1' : stringVal[ i ] = '0';
        intVal = intVal >> 1;
        }

    bool valueRetained = ( intVal == 0 );

    if ( !valueRetained && !allowTruncate )
        ERROR( "Given value cannot be stored in requested number of bits" );
    else
        m_value = stringVal;

    return valueRetained;
}


size_t Nbinary::getMinBitLength( const unsigned long long value )
{
    size_t bitLen = 0;
    if ( value != 0 )
        bitLen = log2( value ) + 1;
    return bitLen;
}

