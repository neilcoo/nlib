// nserial.cxx by Neil Cooper. See nserial.h for documentation
#include "nserial.h"

#include <fcntl.h>         // File control definitions: open(), O_RDWR, O_NOCTTY
#include <unistd.h>        // UNIX standard function definitions: read(), write(), close()
#include <iostream>        // for ostringstream
#include <sys/ioctl.h>     // for ioctl()

#ifdef __CYGWIN__
#include <sys/socket.h>    // for FIONREAD
#endif

#include <stdio.h>
#include <stdlib.h>

#include "nerror.h"

using namespace std;

Nserial::Nserial( const string         theDevice,        // = /dev/ttyS0
                  const unsigned int   theBaudRate,      // = 9600
                  const unsigned int   theDataBitCount,  // = 8
                  const unsigned char  theParity,        // = 'n'
                  const unsigned int   theStopBitCount,  // = 1
                  const unsigned char  theFlowControl,   // = 'n'
                  const bool           theBlockingReads, // = true
                  const bool           theRawMode,       // = true
                  const bool           theReadEnable,    // = true
                  const bool           theWriteEnable,   // = true
                  const bool           theIgnoreDCD,     // = true
                  const bool           theNoCtty         // = true
                                                         )  :  m_handle( 0 ),
                                                               m_device( theDevice )
{
    int flags = 0;

    if ( theReadEnable )
        if ( theWriteEnable )
            flags = O_RDWR;
        else
            flags = O_RDONLY;
    else
        if ( theWriteEnable )
            flags = O_WRONLY;

    if ( theIgnoreDCD )
        flags = flags | O_NDELAY;

    if ( theNoCtty )
        flags = flags | O_NOCTTY;

    m_handle = open( theDevice.c_str(), flags );

    if ( -1 == m_handle )
        EERROR( "Nserial: Can't open serial port '", theDevice, "'" );

    if ( -1 == tcgetattr( m_handle, &m_attr ) )
        EERROR( "Nserial: Can't get port attributes of '", theDevice, "'." );

    setBaudRate( theBaudRate );
    setDataBits( theDataBitCount );
    setParity( theParity );
    setStopBits( theStopBitCount );
    setFlowControl( theFlowControl );
    setReadTimeouts( 1, 0 );
    setReadBlocking( theBlockingReads );
    setRawMode( theRawMode );

    m_attr.c_cflag |= ( CLOCAL | CREAD );   // CLOCAL = do not change owner of port
                                            // CREAD = enable receiver
    commitChanges( IMMEDIATE );
}


Nserial::~Nserial()
{
    if ( m_handle )
        if ( 0 != close( m_handle ) )
            EWARN( "Nserial: Can't close '", m_device, "'." );
}


void Nserial::tx( const void* theData, const size_t theLength )
{
    ssize_t count = write( m_handle, theData, theLength );
    if ( -1 == count )
        EERROR( "Nserial::Tx: Write to '", m_device, "' failed." );

    if ( count < 0 )
        ERROR( "Nserial::Tx: Write to '", m_device, "' returned unexpected value < 0: ", count );

    if ( (size_t)count != theLength )
        WARN( "Nserial::Tx: Write of length ", theLength,
              " to '", m_device, "' returned unexpected length of ", count, "." );
}


size_t Nserial::rx( void* theData, const size_t theMaxLength )
{
    ssize_t count = read( m_handle, theData, theMaxLength );
    if ( -1 == count )
        EERROR( "Nserial::Rx: Read from '", m_device, "' failed." );

    return count;
}


void Nserial::sendBreak( const int theDurationMs )
{
    if ( tcsendbreak( m_handle, theDurationMs ) != 0 )
        EERROR( "Nserial::SendBreak: tcsendbreak() on '", m_device, "' failed." );
}


void Nserial::drain()
{
    if ( tcdrain( m_handle ) != 0 )
        EERROR( "Nserial::Drain: tcdrain() on '", m_device, "' failed." );
}


void Nserial::flush( const bool flushRx, const bool flushTx )
{
    int queueId = 0;

    if ( flushRx )
        {
        queueId = TCIFLUSH;
        if ( flushTx )
        queueId = TCIOFLUSH;
        }
    else
        if ( flushTx )
            queueId = TCOFLUSH;

    if ( flushRx || flushTx )
        if ( tcflush( m_handle, queueId ) != 0 )
            EERROR( "Nserial::Flush tcflush() on '", m_device, "' failed." );
}


void Nserial::suspendFlow( const bool sendStop )
{
    if ( tcflow( m_handle, sendStop ? TCOOFF : TCIOFF ) != 0 )
        EERROR( "Nserial::SuspendFlow tcflow() on '", m_device, "' failed." );
}


void Nserial::resumeFlow( const bool sendStart )
{
    if ( tcflow( m_handle, sendStart ? TCOON : TCION ) != 0 )
        EERROR( "Nserial::ResumeFlow tcflow() on '", m_device, "' failed." );
}


int Nserial::bytesAvailable()
{
    int bytes = 0;

    ioctl( m_handle, FIONREAD, &bytes );
    return bytes;
}


void Nserial::setBaudRate( const unsigned int theBaudRate )
{
// #define NEW_SPEEDSET_METHOD 1

#ifdef NEW_SPEEDSET_METHOD

    // This method is apparently not currently supported under linux, however
    // if it ever is, use this instead as (in theory) it also allows custom baud rates.

    m_attr.c_cflag &= ~CBAUD;
    m_attr.c_cflag |= BOTHER; // either BOTHER or CBAUDEX ?
    m_attr.c_ispeed = theBaudRate;
    m_attr.c_ospeed = theBaudRate;

#else

    speed_t baud = 0;

    switch ( theBaudRate )
        {
        case 0:     // hang up
            baud = B0;
            break;

        case 50:
            baud = B50;
            break;

        case 75:
            baud = B75;
            break;

        case 110:
            baud = B110;
            break;

        case 134:
            baud = B134;
            break;

        case 150:
            baud = B150;
            break;

        case 200:
            baud = B200;
            break;

        case 300:
            baud = B300;
            break;

        case 600:
            baud = B600;
            break;

        case 1200:
            baud = B1200;
            break;

        case 1800:
            baud = B1800;
            break;

        case 2400:
            baud = B2400;
            break;

        case 4800:
            baud = B4800;
            break;

        case 9600:
            baud = B9600;
            break;

        case 19200:
            baud = B19200;
            break;

        case 38400:
            baud = B38400;
            break;

        case 57600:
            baud = B57600;
            break;

        case 115200:
            baud = B115200;
            break;

        case 230400:
            baud = B230400;
            break;

        case 460800:
            baud = B460800;
            break;

        case 500000:
            baud = B500000;
            break;

        case 576000:
            baud = B576000;
            break;

        case 921600:
            baud = B921600;
            break;

        case 1000000:
            baud = B1000000;
            break;

        case 1152000:
            baud = B1152000;
            break;

        case 1500000:
            baud = B1500000;
            break;

        case 2000000:
            baud = B2000000;
            break;

        case 2500000:
            baud = B2500000;
            break;

        case 3000000:
            baud = B3000000;
            break;

#ifndef __CYGWIN__ // The following 2 don't seem to be defined under Cygwin
        case 3500000:
            baud = B3500000;
            break;

        case 4000000:
            baud = B4000000;
            break;
#endif

        default:
            ERROR( "Nserial::SetBaudRate: Baudrate ", theBaudRate, " invalid." );
        }

    cfsetispeed( &m_attr, baud );
    cfsetospeed( &m_attr, baud );
#endif
}


void Nserial::setDataBits( const unsigned int  theDataBitCount )
{
    m_attr.c_cflag &= ~CSIZE;

    switch ( theDataBitCount )
        {
        case 5:
            m_attr.c_cflag |= CS5;
            break;

        case 6:
            m_attr.c_cflag |= CS6;
            break;

        case 7:
            m_attr.c_cflag |= CS7;
            break;

        case 8:
            m_attr.c_cflag |= CS8;
            break;

        default:
            EERROR( "Nserial::SetDataBits: Parameter '", theDataBitCount, "' invalid. Should be 5, 6, 7 or 8." );
        }
}


void Nserial::setStopBits( const unsigned int theStopBitCount )
{
    if ( theStopBitCount == 1 )
        m_attr.c_cflag &= ~CSTOPB;
    else
        if ( theStopBitCount == 2 )
            m_attr.c_cflag |= CSTOPB;
        else
            EERROR( "Nserial::SetStopBits: Parameter '", theStopBitCount, "' invalid. Should be 1 or 2." );
}


void Nserial::setParity( const char theParity )
{
    switch( theParity )
        {
        case 'n':
        case 'N':
            m_attr.c_cflag &= ~PARENB;
            break;

        case 'e':
        case 'E':
            m_attr.c_cflag |= PARENB;
            m_attr.c_cflag &= ~PARODD;
            break;

        case 'o':
        case 'O':
            m_attr.c_cflag |= PARENB;
            m_attr.c_cflag |= PARODD;
            break;

        default:
            EERROR( "Nserial::SetParity: Parameter '", theParity, "' invalid. Should be one of \"nNoOeE\"." );
        }
}

void Nserial::setFlowControl( const unsigned char theflowControlType )
{
    m_attr.c_iflag &= ~(IXON | IXOFF | IXANY | CRTSCTS );

    switch ( theflowControlType )
        {
        case 'h':
        case 'H':
            m_attr.c_cflag |= CRTSCTS;
            break;

        case 's':
        case 'S':
            m_attr.c_iflag |= (IXON | IXOFF | IXANY);
            break;

        case 'n':
        case 'N':
            break;

        default:
            EERROR( "Nserial::SetFlowControl: Parameter '", theflowControlType, "' invalid. Should be one of \"hHsSnN\"." );
        }
}


void Nserial::setReadBlocking( const bool theBlockingFlag )
{
    if ( theBlockingFlag )
        fcntl( m_handle, F_SETFL, 0 );
    else
#if defined( __ANDROID__ )
        fcntl( m_handle, F_SETFL, O_NDELAY );  // FNDELAY note defined under Termux. O_NDELAY is an untested guess
#else
        fcntl( m_handle, F_SETFL, FNDELAY );
#endif
}


void Nserial::setRawMode( const bool theRawFlag )
{
    // ECHO - echo input characters
    // ECHOE - echo erase character as BS-SP-BS
    // ISIG  - Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
    // OPOST - Postprocess output
    // See here for more: https://people.na.infn.it/~garufi/didattica/CorsoAcq/SerialProgrammingInPosixOSs.pdf

    // TODO: should probably move ECHO/ECHOE, ISIG etc into their own accessors
    if ( theRawFlag )
        {
        m_attr.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG );
        m_attr.c_oflag &= ~OPOST;
        }
    else
        {
        m_attr.c_lflag |= ( ICANON | ECHO | ECHOE );
        m_attr.c_oflag |= OPOST;
        }
}


void Nserial::setReadTimeouts(   const unsigned char theVmin,
                                 const unsigned char theVtime )
{
    m_attr.c_cc[ VMIN ] = theVmin;
    m_attr.c_cc[ VTIME ] = theVtime;
}


void Nserial::commitChanges( const COMMIT_TIME time )
{
    int attrTime = TCSANOW;

    if ( time == AFTER_TX_EMPTY )
        attrTime = TCSADRAIN;
    else
        if ( time == AFTER_TX_AND_FLUSH )
            attrTime = TCSAFLUSH;

    if ( -1 == tcsetattr( m_handle, attrTime, &m_attr ) )
        EERROR( "Nserial::CommitChanges: tcsetattr() on '", m_device, "' failed." );
}


int Nserial::getControlSignals()
{
    int status;

    if ( -1 == ioctl( m_handle, TIOCMGET, &status ) )
        EERROR( "Nserial::GetControlSignals: ioctl( TIOMCGET ) on '", m_device, "' failed." );

    return status;
}


void Nserial::setControlSignals( const int theSignals )
{
    int sig = theSignals;

    if ( -1 == ioctl( m_handle, TIOCMSET, &sig ) )
        EERROR( "Nserial::SetControlSignals: ioctl( TIOMCSET ) on '", m_device, "' failed." );
}

