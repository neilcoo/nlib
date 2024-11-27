// nconfig.cxx by Neil Cooper. See nconfig.h for documentation
#include "nconfig.h"

#include <string>
#include <fstream>

#include "nerror.h"

using namespace std;

Nconfig::Nconfig( const char* theConfigFile,
                  const bool  theUniqueFlag,
                  const char* theTerminators,
                  const char* theCommentors  )
{
    m_fileName = theConfigFile;
    m_uniqueFlag = theUniqueFlag;
    ifstream inFile( theConfigFile );

    if ( inFile.fail() )
        ERROR(  "Nconfig: Can't open configuration file: '", theConfigFile, "'" );

    unsigned long lineNo = 1;
    while ( !inFile.eof() )
        {
        if ( !inFile.good() )
            ERROR( "Nconfig: Unknown I/O failure other than EOF while reading configuration file: '", theConfigFile, "' at line ", lineNo );

        string inLine;
        getline( inFile, inLine );
        if ( !parseLine( inLine, theTerminators, theCommentors ) )
            ERROR( "Nconfig: Syntax error in configuration file: '", theConfigFile, "' at line ", lineNo );

        lineNo++;
        }
}


Nconfig::~Nconfig()
{
}


bool Nconfig::getValue( const char* theTag, string& theValue )
{
    CONFIG_MAP_TYPE::const_iterator iter( m_config.find( theTag ) );
    const bool found( iter != m_config.end() );
    if ( found )
        theValue = iter->second;

    return ( found );
}


bool Nconfig::getValue( const string& theTag, string& theValue )
{
    return ( getValue( theTag.c_str(), theValue ) );
}


bool Nconfig::eraseValue( const string& theTag )
{
    return ( m_config.erase( theTag ) > 0 );
}


bool Nconfig::parseLine( const string& theLine,
                         const char*   theTerminators,
                         const char*   theCommentors   )
{
    static const string terminators( theTerminators );
    static const string commentors( theCommentors );

    string work = theLine;
    size_t index = 0;
    size_t len = theLine.length();
    bool quoted = false;
    bool slashed = false;
    bool comment = false;
    bool tagParsed = false;
    bool valueParsed = false;
    bool parsedOk = true;
    string tag;
    string value;


    while ( ( index < len ) &&
            parsedOk       &&
            ( !comment ) )
        {
        bool skip = false;
        bool slashSet = false;
        char c = theLine[ index ];

        // Handle quotes
        if ( ( c == '"' ) && ( !slashed ) )
            {
            quoted = !quoted;
            skip = true;
            }

        // Skip unslashed/unquoted whitespace until tag/value has length
        if ( ( !skip ) &&( c == ' ' || c=='\t' ) && ( !quoted ) && ( !slashed ) )
            skip = tagParsed ? ( value.length() == 0 ) : ( tag.length() == 0 );

        // Detect and skip comments
        if (    ( !skip ) &&
                ( commentors.find_first_of( c ) != string::npos ) &&
                ( !quoted ) &&
                ( !slashed ) )
            {
            comment = true;
            skip = true;
            }

        // Check for extra fields
        if ( ( !skip ) && ( c != ' ' ) && ( c != '\t' ) && ( !comment ) && tagParsed && valueParsed )
            {
            parsedOk = false;
            skip = true;
            }

        // Check for unslashed/unquoted terminator
        if (    ( !skip ) &&
                ( terminators.find_first_of( c ) != string::npos ) &&
                ( !quoted ) &&
                ( !slashed ) )
            {
            if ( !tagParsed )
                tagParsed = true;
            else
                if ( !valueParsed )
                    valueParsed = true;
                else
                    parsedOk =  ( ( c == ' ' ) || ( c == '\t' ) || ( comment ) );
            skip = true;
            }

        // Detect slash
        if ( ( !skip ) && ( c == '\\' ) && ( !slashed ) )
            {
            slashed = true;
            slashSet = true;
            skip = true;
            }

        // Only reset slashed if we didn't just set it
        if ( !slashSet )
            slashed = false;

        if ( !skip )
            if ( !tagParsed )
                tag += c;
            else
                value += c;

        index++;
        }

    bool blankLine = ( ( tag.length() == 0 ) && ( value.length() == 0 ) );

    parsedOk = (    parsedOk                                &&
                    ( !quoted )                             &&
                    ( !slashed )                            &&
                    ( blankLine || ( ( tag.length() > 0 )   &&
                    ( value.length() > 0 ) ) ) );

    if ( !blankLine && parsedOk )
        {
        if ( m_uniqueFlag )
            {
            string temp;
            if ( getValue( tag, temp ) )
                ERROR( "Nconfig: Duplicate parameter ", tag, " found in config file ", m_fileName );
            }
        m_config[ tag ] = value;
        }

    return parsedOk;
}

