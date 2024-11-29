#include "nsocketCan.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "nerror.h"

using namespace std;

NsocketCan::NsocketCan(const std::string device, const bool rxAllCanInterfaces ) :
    m_socket(0)
{
    m_device = device;
    m_socket = socket( PF_CAN, SOCK_RAW, CAN_RAW );
    if ( m_socket < 0 )
        EERROR("Can't open CAN socket on device ", device );

    struct ifreq ifr;
    ifr.ifr_ifindex = 0;  // interface index of 0 = rx on all CAN interfaces

    if ( !rxAllCanInterfaces )
        {
        strcpy( ifr.ifr_name, device.c_str() );
        if ( ioctl( m_socket, SIOCGIFINDEX, &ifr ) )
            EERROR("Can't retrieve interface index for device ", device );
        }

    // bind socket to CAN interface(s)
    struct sockaddr_can addr;
    memset( &addr, 0, sizeof( addr ) );
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if ( bind( m_socket, (struct sockaddr *)&addr, sizeof( addr) ) < 0 )
        if ( rxAllCanInterfaces )
            EERROR("Can't bind socket to CAN interface for all CAN devices" );
        else
            EERROR("Can't bind socket to CAN interface for device ", device );
}


NsocketCan::~NsocketCan()
{
    if ( close( m_socket ) < 0 )
       EWARN("Can't close CAN socket for device ", m_device );

}


void NsocketCan::setRxFilter( const FILTER_LIST filterList )
{
    struct can_filter* filter = new struct can_filter[ filterList.size() ];

    for ( size_t i=0; i < filterList.size(); i++ )
        {
        filter[i].can_id = filterList[i];
        filter[i].can_mask = filterList[i];
        }

    int setStatus = setsockopt( m_socket, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof( filter ) );

    delete[] filter;

    if ( setStatus != 0 )
        EERROR("Can't set CAN filter for device ", m_device );
}


void NsocketCan::tx( const uint32_t  canId,
                     const CAN_DATA  data )
{
    size_t dataSize = data.size();

    if ( dataSize > 8 )
        ERROR( "NsocketCan::Tx data parameter too long: must be 8 or less." );

    struct can_frame frame;
    frame.can_id = canId;
// Frame.can_dlc got deprecated sometime between linux kernel 5.10 and 5.15
// Those versions of /usr/include/linux/can.h don't include a definition for
// can_frame.len8_dlc, and there is no version number in can.h so we need to
// use other changes to detect if can_frame.len8_dlc is defined in the current
// environment or not.
// 2 other changes we can test for in the newer header (that supports
// can_frame.len8_dlc) is that #defines for CAN_MAX_RAW_DLC and CANFD_FDF got added.
// Let's try testing for CAN_MAX_RAW_DLC for now. if this doesn't work right for
// intermediate versions of can.h, maybe try testing for CANFD_FDF instead.
    frame.can_dlc = dataSize;   // When can_dlc is deprecated, does using it hurt?
#ifdef CAN_MAX_RAW_DLC
	frame.len8_dlc = dataSize;  // len8_dlc is the new version of can_dlc
#endif

    for ( size_t i=0; i < dataSize; i++ )
        frame.data[i] = data[i];

    size_t frameSize = sizeof( frame );

    ssize_t sentLen = write( m_socket, &frame, frameSize );
    if ( sentLen == -1 )
        EERROR("Can't write to CAN device ", m_device );

    if ( sentLen < 0 )
        ERROR( "Write to CAN device returned unexpected value < 0: ", sentLen );

    if ( (size_t)sentLen != frameSize )
        ERROR("Write to CAN device ", m_device, " was short: (", sentLen, " bytes instead of ", frameSize, ")" );
}


void NsocketCan::tx( const CAN_DATA      data,
                     const uint32_t      canId,
                     const FRAME_FORMAT  format,
                     const FRAME_TYPE    type,
                     const bool          remoteTransmissionRequest )
{
    canid_t id = 0;

    if ( format == EXTENDED_FRAME )
        id = ( canId & EXTENDED_FRAME_ID_MASK ) | EXTENDED_FRAME_FLAG;
    else
        id = canId & STANDARD_FRAME_ID_MASK;

    if ( type == ERROR_FRAME )
        id = id | ERROR_FRAME_FLAG;

    if ( remoteTransmissionRequest )
        id = id | RTR_FRAME_FLAG;

    tx( id, data );
}


NsocketCan::CAN_ID NsocketCan::rx( CAN_DATA& data )
{
    struct can_frame frame;
    size_t frameSize = sizeof( frame );

    ssize_t readLen = read( m_socket, &frame, frameSize );
    if ( readLen  == -1 )
        EERROR("Can't read from CAN device ", m_device );

    if ( readLen < 0 )
        ERROR( "Read from CAN device returned unexpected value < 0: ", readLen );

    if ( (size_t)readLen != frameSize )
        ERROR("Read from CAN device ", m_device, " was short: (", readLen, " bytes instead of ", frameSize, ")" );

    data.clear();
    for ( size_t i = 0; i < frame.can_dlc; i++ )
        data.push_back( frame.data[i] );

    return frame.can_id;
}


void NsocketCan::extractFlags(  const uint32_t canIdMsg,
                                uint32_t& canId,
                                bool& errorFrame,
                                bool& remoteTransmissionRequest,
                                bool& extendedFrame )
{
    errorFrame = ( canIdMsg & ERROR_FRAME_FLAG ) > 0;
    remoteTransmissionRequest = ( canIdMsg & RTR_FRAME_FLAG ) > 0;
    extendedFrame = ( canIdMsg & EXTENDED_FRAME_FLAG ) > 0;

    if ( extendedFrame )
        canId = canIdMsg & EXTENDED_FRAME_ID_MASK;
    else
        canId = canIdMsg & STANDARD_FRAME_ID_MASK;
}


void NsocketCan::setMessageLoopback( const bool loopbackEnabled )
{
    int loopback = loopbackEnabled ? 1 : 0;
    if ( setsockopt( m_socket, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback) ) != 0 )
        EERROR("Can't set 'message loopback' for device ", m_device );
}


void NsocketCan::setRxOwnMessages( const bool rxOwnMessages )
{
    int rxMessages = rxOwnMessages ? 1 : 0;
    if ( setsockopt( m_socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &rxMessages, sizeof(rxMessages) ) != 0 )
        EERROR( "Can't set 'receive own messages' for device ", m_device );
}
