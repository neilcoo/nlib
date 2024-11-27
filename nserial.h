#ifndef NSERIAL_H
#define NSERIAL_H

//  Nserial v1.2 by Neil Cooper 31 October 2017
//  Encapsulates an RS-232-type serial port device.

#include <stddef.h>
#include <termios.h>    // POSIX terminal control definitions: struct termios, speed_t
#include <sys/ioctl.h>  // for TIOCM_*

#include <string>

class Nserial
{
public:


typedef enum
{
    IMMEDIATE,
    AFTER_TX_EMPTY,
    AFTER_TX_AND_FLUSH

}  COMMIT_TIME;

#ifndef __CYGWIN__
enum CONTROL_SIGNALS
{
    RTS   = TIOCM_RTS,   // Request to Send
    CTS   = TIOCM_CTS,   // Clear To Send
    DTR   = TIOCM_DTR,   // Data Terminal Ready
    DSR   = TIOCM_DSR,   // Data Set Ready
    LE    = TIOCM_LE,    // Data Set Ready/Line Enable
    DCD   = TIOCM_CAR,   // Data Carrier Detect
    RI    = TIOCM_RNG,   // Ring Indicator
    ST    = TIOCM_ST,    // Secondary TXD (transmit)
    SR    = TIOCM_SR     // Secondary RXD (receive)
};
#endif

    Nserial( const char*          theDevice         = "/dev/ttyS0", // Serial device
             const unsigned int   theBaudRate       = 115200,       // Baud rate
             const unsigned int   theDataBitCount   = 8,            // No. of data bits
             const unsigned char  theParity         = 'n',          // Parity
             const unsigned int   theStopBitCount   = 1,            // No. of stop bits
             const unsigned char  theFlowControl    = 'n',          // HW, SW or no flow control
             const bool           theBlockingReads  = true,         // Rx() blocks until data available
             const bool           theRawMode        = true,         // See below for decription of remaining params
             const bool           theReadEnable     = true,
             const bool           theWriteEnable    = true,
             const bool           theIgnoreDCD      = true,
             const bool           theNoCtty         = true           );

// theDevice:        Physical serial port to open.
//
// theBaudRate:      Speed of serial port. Supported speeds are:
//                   0 ( hang up ), 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,
//                   2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800,
//                   500000, 576000, 921600, 1000000, 1152000, 1500000, 2000000,
//                   2500000, 3000000, 3500000, 4000000
//                   Note: 3500000 and 4000000 are not supported under Cygwin
//
// theDataBitCount:  No. of data bits. Vaid values are 5, 6, 7 or 8.
//
// theParity:        N or n = None, E or e = even, O or o = odd.
//
// theStopBitCount:  No. of stop bits. Valid values are 1 or 2.
//
// theFlowControl:   Flow control method: N or n = none,
//                                        H or h = hardware (RTS/CTS)
//                                        S or s = software (XON/XOFF)
//
// theBlockingReads: If true, calls to Rx() will block while incoming data not available.
//                   If false, calls to Rx() may return 0.
//
// theRawMode:       If true, port is in raw mode, else port is in canonical mode.
//                   Raw mode means no translation of data. Canonical mode is line-oriented
//                   and intended for terminals. Incoming characters are put into a buffer
//                   which may be edited by the user until CR or LF is received.
//
// theReadEnable:    If false, reading is not enabled on the port.
//
// theWriteEnable:   If false, writing is not enabled on the port.
//
// theIgnoreDCD:     If false, process will sleep until DCD signal line is at space voltage.
//
// theNoCtty:        If false, keyboard abort signals etc. will affect process.

    virtual ~Nserial();

    virtual void tx( const void* theData, const size_t theLength );
    // Transmits the given data.
    // theData = pointer to data.
    // theLength = Length in bytes of data to transmit.

    virtual size_t rx( void* theBuffer, const size_t theBufferSize );
    // Receives available data from RS-232 port into given buffer.
    // theBuffer = pointer to buffer t hold data
    // theBufferSize = Maximum size of data to read, in bytes.
    // Return: No of bytes read.

    virtual void sendBreak( const int theDurationMs = 0 );
    // If port is configured for asynchronous data tx, Transmits a
    // continuous stream of zero-valued bits for the specified duration.
    // If duration is zero, it transmits zero-valued bits for at least 250 Ms,
    // and not more than 500 Ms.
    // If port is not configured for asynchronous data tx, performs no action.

    virtual void drain();
    // Blocks until all data queued ( with Tx() ) has been actually sent.

    virtual void flush( const bool flushRx, const bool flushTx );
    // Flush data in kernel/device buffers.
    // If flushRx is true, flush any data that has not been read yet ( with Rx() ).
    // If flushTx is true, flush any data has not been actually sent by the port yet.

    virtual void suspendFlow( const bool sendStop = false );
    // Manual flow control of port.
    // If sendStop is true, sends a stop character (i.e. XOFF).

    virtual void resumeFlow( const bool sendStart = false );
    // Resumes dataflow of port folloiwng a call to SuspendFlow().
    // If sendStart is true, sends a start character (i.e. XON).

    int bytesAvailable();
    // Returns the number of bytes avaialble to be read ( with Rx() ).

    void setBaudRate( const unsigned int theBaudRate );
    // Sets baud rate used by the port to that given. Not effective until Commit() is called.
    // See constructor's comment for supported values.

    void setDataBits( const unsigned int  theDataBitCount = 8 );
    // Sets data length used by the port to that given. Not effective until Commit() is called.
    // See constructor's comment for supported values.

    void setStopBits( const unsigned int   theStopBitCount = 1 );
    // Sets stop bits used by the port to that given. Not effective until Commit() is called.
    // See constructor's comment for supported values.

    void setParity( const char theParity = 'n' );
    // Sets parity used by the port to that given. Not effective until Commit() is called.
    // See constructor's comment for supported values.

    void setFlowControl( const unsigned char theflowControlType );
    // Sets flow control used by the port to that given. Not effective until Commit() is called.
    // See constructor's comment for supported values.

    void setReadBlocking( const bool theBlockingFlag = true );
    // Sets read blocking used by the port to that given. Not effective until Commit() is called.
    // See constructor's comment for supported values.

    void setRawMode( const bool theRawFlag = true );
    // Sets mode used by the port to that given. Not effective until Commit() is called.
    // See constructor's comment for supported values.

    void setReadTimeouts( const unsigned char theVmin,
                          const unsigned char theVtime );
    // Sets read timeouts used by the port to that given. Not effective until Commit() is called.
    // Basically, this is used to block incoming data into expected length chunks per call to rx(),
    // assuming there is some longer delay beteween the remote end sending each chunk than ever
    // seen between each byte of a chunk.
    // Vmin:    Minimum no. of bytes to wait to receive before returning.
    // Vtime:   No. of 10ths of a second to wait with no bytes received before returning.
    //          Vtime timer is reset on reception of each byte.
    // VMIN = 0 and VTIME = 0: This is a completely non-blocking read.
    // VMIN = 0 and VTIME > 0: This is a pure timed read. Note that this is an overall timer, not an intercharacter one.
    // VMIN > 0 and VTIME > 0: Satisfied when either VMIN characters have been transferred to the caller's buffer, or
    //                         when VTIME expires between characters. Since this timer is not started until the first
    //                         character arrives, this call can block indefinitely if the serial line is idle.
    //                         VTIME is considered to be an intercharacter timeout, not overall. This call should never
    //                         return zero bytes read.
    // VMIN > 0 and VTIME = 0: Counted read satisfied only when at least VMIN characters have been read.


    void commitChanges( const COMMIT_TIME time );
    // Apply the configuration changes made.
    // Parameter:  IMMEDIATE            - Apply changes immediately
    //             AFTER_TX_EMPTY       - Apply changes when the Tx buffer is empty.
    //             AFTER_TX_AND_FLUSH   - Apply changes and flush input buffer when Tx bufer is empty

    int getControlSignals();
    // Return a bitmask (of CONTROL_SIGNALS enums) corresponding to the current state of the port's signals/pins.

    void setControlSignals( const int theSignals );
    // Set the the port's control signals/pins to that given in the bitmask (of CONTROL_SIGNALS enums).
    // 0 = off 1 = on.

private:

   int            m_handle;
   std::string    m_device;
   struct termios m_attr;
};

#endif

