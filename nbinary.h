#ifndef NBINARY_H
#define NBINARY_H

//  Nbinary v1.1 by Neil Cooper 11th July 2013
//  String-based arbitrary length binary object.

#include <string>

class Nbinary
{
public:
    Nbinary();
    Nbinary( const Nbinary& other );
    Nbinary( const std::string& strValue ); // may be 0xFFF, 0b10101, 1234 etc
    Nbinary( const char* strValue );        // may be 0xFFF, 0b10101, 1234 etc
    Nbinary( const unsigned long long intValue, const size_t lengthInbits = 0, const bool allowTruncate = false );
    //  intValue:       Value to store.
    //  lengthInBits:   No of bits to use to represent. If 0, just use the minimum possible.
    //  allowTruncate:  If allowTruncate is false and the given value is too
    //                      large to be stored in the given bit length, an exception wll be thrown.
    virtual ~Nbinary();

    virtual bool initialise( const unsigned long long value, const size_t lengthInbits = 0, const bool allowTruncate = false );
    // Set to a (decimal) value of given bit length. If specified bit length is
    // too short to represent given value, stored value will be truncated to given
    // given length and return value will be false.
    // Parameters:
    //  value:          Value to store.
    //  lengthInbits:   No. of bits in desired stored value.
    //  allowTruncate:  If allowTruncate is false and the given value is too
    //                  large to be stored in the given bit length, an exception wll be thrown.
    // Return:
    //  true = Success.
    //  false if allowTruncate is true and the given bit length is too short to store the entire
    //  value. (Value truncated).

    virtual bool setBitLength( const size_t lengthInbits, const bool allowTruncate = false );
    // Change bit length of stored value. If increased, the stored value will be
    // left-padded with 0's.
    // Parameters:
    //    lengthInbits:   No. of bits in desired stored value.
    //  allowTruncate:  If allowTruncate is false and the given value is too
    //                  large to be stored in the given bit length, an exception wll be thrown.
    // Return:
    //  true = Success.
    //  false if allowTruncate is true and the given bit length is too short to store the entire
    //  value. (Value truncated).

    virtual std::string getAsBinary() const;
    // Returns string containing binary representation of stored value.
    // Return:
    //    String made up only of '0' and/or '1' characters.

    virtual std::string getAsHex() const;
    // Returns string containing hex representation of stored value.
    // Return:
    //    String is value only (no leading "0x").

    virtual unsigned long long getAsInt() const;
    // Returns integer containing stored value.
    // Return:
    //    Integer containing stored value.

    virtual void setByString( const std::string& value );
    // if value prefixed by "0x" or 0X" call SetByHex()
    // if value prefixed by "0b" or 0B" call SetByBinary()
    // else call setByInt using the value of the string, and
    // calculating bitlength of the binary from the value itself.

    virtual void setByHex( const std::string& inHex );
    // Set stored value to given hex value.
    // Parameters:
    //    inHex: Hex value to store (do not prefix string with 0x).

    virtual void setByBinary( const std::string& inBinary );
    // Set stored value to given binary value.
    // Parameters:
    //    inBinary: String made up only of '0' and/or '1' characters.

    virtual bool setByInt( const unsigned long long value, const size_t lengthInbits = 0, const bool allowTruncate = false );
    // Set stored value to given integer value.
   // Parameters:
    //  value:          Value to store.
    //  lengthInbits:   No. of bits in desired stored value.
    //  allowTruncate:  If allowTruncate is false and the given value is too
    //                  large to be stored in the given bit length, an exception wll be thrown.
    // Return:
    //  true = Success.
    //  false if allowTruncate is true and the given bit length is too short to store the entire
    //  value. (Value truncated).

    virtual size_t length() const;
    // Get length in bits of stored value.
    // Return:
    //    Length in bits of stored value.

    Nbinary& operator = ( const std::string& value );

    Nbinary& operator = ( const unsigned long long value );

    bool operator == (const Nbinary& other ) const;

    const char operator[] ( const int index ) const;
    char& operator[] ( const int index );
    // Array-style operator to access individual bits of the stored value.
    // N.B: Value is represented in ASCII so uses '0' and '1' instead of literal 0 and 1

private:
    size_t getMinBitLength( const unsigned long long value );

    std::string m_value;
};

#endif

