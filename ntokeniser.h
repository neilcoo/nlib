#ifndef NTOKENISER_H
#define NTOKENISER_H

// Ntokeniser v1.0 by Neil Cooper 30th June 2017
// Implements simple std::string-based tokeniser
//
// Example:
//    std::string tokenStream = "..a.b, .c";
//    std::string token;
//    do {
//       token = Ntokeniser::ConsumeToken( tokenStream, "., " );
//       std::cout << "Token is '" << token << "'" << std::endl;
//       } while ( !token.empty() );
//
// Output:
// Token is 'a'
// Token is 'b'
// Token is 'c'
// Token is ''

#include <string>

class Ntokeniser
{
public:
    static const std::string DEFAULT_TOKEN_TERMINATORS;

    // Each call to ConsumeToken() will consume a token and (only) its terminator from theTokenStream, and return it without the terminator.
    // If ConsumeToken() is called with theTokenStream being empty or only containing terminators, the string returned will be empty.
    // Note: Subsequent calls on the same string may use different terminators. This means that in the case where
    // theTokenStream is terminated with multiple terminators, it is possible for them to not be all consumed in a single call.
    // Consequently, when parsing a stream in a loop, rely on an empty return value rather than an empty tokenStream.
    // Ntokeniser::DEFAULT_TOKEN_TERMINATORS are " ,\t\f\v\n\r"

    static std::string consumeToken( std::string& theTokenStream, const std::string& theTerminators = DEFAULT_TOKEN_TERMINATORS );
};

#endif
