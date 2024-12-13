// nerror.cxx by Neil Cooper. See nerror.h for documentation
#include "nerror.h"

#include <string>
#include <string.h>  // for strerror()

#include "nmutex.h"

using namespace std;

static Nmutex printGuard;

#define Print( msg... )       \
{                             \
   printGuard.lock();         \
   cout << msg << endl;       \
   printGuard.unlock();       \
}

#define PrintErr( msg... )    \
{                             \
   printGuard.lock();         \
   cerr << msg << endl;       \
   printGuard.unlock();       \
}


void Nerror::useSysLog( const bool theUseSysLogFlag )
{
   m_useSysLog = theUseSysLogFlag;
}


bool Nerror::isUsingSysLog()
{
   return m_useSysLog;
}


static std::string getErrnoMessage(const int theErrno)
{
   // The GNU C Library uses a buffer of 1024 characters for strerror()
   static const int STRERROR_BUFFER_SIZE = 1024;

   char tmpBuf[STRERROR_BUFFER_SIZE];
   std::string msg;

   // strerror_r() comes in 2 variants depending on system library version
   // and whether we have explicity defined _GNU_SOURCE or not.
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined _GNU_SOURCE
   // XSI-compliant variant
   int retVal = strerror_r(theErrno, tmpBuf, STRERROR_BUFFER_SIZE);
   if ( retVal == 0 )
      msg = tmpBuf;
   else
      {
      // Handle Glibc versions < 2.13 where strerror_r can set errno
      if ( retVal == -1 )
         retVal = errno;

      std::ostringstream osm;
      osm << "getErrnoMessage:: strerror_r() returned error " << retVal;
      msg = osm.str();
    }
#else
    // GNU-specific variant
    msg = strerror_r( theErrno, tmpBuf, STRERROR_BUFFER_SIZE );
#endif
    return msg;
}


static void makeMessage( const string& theMessage, const int theErrno, string& theBuffer )
{
   theBuffer = theMessage;
   if ( theErrno )
      {
      if ( theBuffer.length() && ( theBuffer[ theBuffer.length() - 1 ] == '.' ) )
         theBuffer.pop_back();
      theBuffer += ": ";
      theBuffer += getErrnoMessage( theErrno );
      theBuffer += ".";
      }
}


string Nerror::compose2(std::ostringstream& msgBuf)
{
   return "";
}


// Note 1:
// For debug mode calls, the syslog call option parameter should strictly
// include a LOG_DEBUG flag, however the syslogd default setting is that
// LOG_DEBUG output does not appear in the log.
// To avoid confusion I've therefore decided to omit the LOG_DEBUG flag.
void Nerror::warn(   const string&        theMessage,
                     const char*          theFilename,
                     const unsigned long  theLineNo,
                     const int            theErrno    )
{
   string message;
   makeMessage( theMessage, theErrno, message );

   if ( isUsingSysLog() )
      syslog(  LOG_WARNING,   // strictly we should add LOG_DEBUG here. See Note 1 above
               "Warning in file %s line %lu: %s",
               theFilename, theLineNo , message.c_str() );
   else
      PrintErr( "Warning in file " << theFilename  << " line " << theLineNo << ": " << message );
}


void Nerror::warn(   const string&  theMessage,
                     const int      theErrno   )
{
   string message;
   makeMessage( theMessage, theErrno, message );

   if ( isUsingSysLog() )
      syslog( LOG_WARNING, "Warning: %s", message.c_str() );
   else
      PrintErr( "Warning: " << message );
}


void Nerror::log( const string&        theMessage,
                  const char*          theFilename,
                  const unsigned long  theLineNo,
                  const int            theErrno    )
{
   string message;
   makeMessage( theMessage, theErrno, message );

   if ( isUsingSysLog() )
      syslog(  LOG_INFO,   // strictly we should add LOG_DEBUG here. See note 1 above
               "File %s line %lu: %s",
               theFilename, theLineNo , message.c_str()  );
   else
      Print( "File " << theFilename   << " line " << theLineNo << ": " << message );
}


void Nerror::log( const string& theMessage,
                  const int     theErrno   )
{
   string message;
   makeMessage( theMessage, theErrno, message );

   if ( isUsingSysLog() )
      syslog( LOG_INFO, "%s", message.c_str() );
   else
      Print( message );
}


NerrorException::NerrorException(   const std::string&   theErrorMsg,
                                    const char*          theFileName,
                                    unsigned long        theLineNo,
                                    const int            theErrno    ) :   m_fileName( theFileName ),
                                                                           m_lineNo( theLineNo ),
                                                                           m_errno( theErrno )

{
   makeMessage( theErrorMsg.c_str(), theErrno, m_msg );
}

#ifdef NERROR_USE_SYSLOG
   bool Nerror::m_useSysLog = true;
#else
   bool Nerror::m_useSysLog = false;
#endif

