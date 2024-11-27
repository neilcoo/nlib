#ifndef NCONFIG_H
#define NCONFIG_H

// nconfig v1.1 by Neil Cooper 21st March 2011
// Implements a very basic "tag,value"-style configuration file reader.

// Rules covering configuration file format and contents:
// One tag value pair per line.
// Tags and values to be separated by a terminator (a comma or one or more spaces by default).
// Comments (starting with # or / by default) make the rest of the line ignored.
// Whitespace will be trimmed from the start and end of tags and values.
// " or ' characters may be used around tags or values to make them literal.
// The / character 'literalises' the following character.

#include <map>
#include <string>

class Nconfig
{
public:
    Nconfig(    const char* theConfigFile,
                const bool  theUniqueFlag = true,
                const char* theTerminators = " ,\t",
                const char* theCommentors = "#/"    );
    // constructor
    // parameters:
    // theConfigFile:      File path/name of configuration file.
    // theUniqueTagsFlag:  If true: ERROR if duplicate tag in config file is encountered.
    // theTerminators:     All characters that act as terminators for tags and values.
    // theCommentors:      All characters that when found as the first non-whitespace character
    //                     on a line cause it to be ignored.

    virtual ~Nconfig();

    bool getValue( const char* theTag, std::string& theValue );

    bool getValue( const std::string& theTag, std::string& theValue );

    bool eraseValue( const std::string& theTag );

private:
    bool parseLine( const std::string& theLine,
                    const char* theTerminators,
                    const char* theCommentors   );

    typedef std::map< std::string, std::string > CONFIG_MAP_TYPE;

    CONFIG_MAP_TYPE  m_config;
    std::string      m_fileName;
    bool             m_uniqueFlag;
};

#endif
