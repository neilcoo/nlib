#pragma once
// NsocketCan v1.0 by Neil Cooper 27th April 2022
// Encapsulates a SocketCAN socket.

#include <string>
#include <vector>
#include <stdint.h>

class NsocketCan
{
public:
    const uint32_t STANDARD_FRAME_ID_MASK = 0x7FF;
    const uint32_t EXTENDED_FRAME_ID_MASK = 0x1FFFFFFF;

    // Flags for use inside ID frame
    const uint32_t ERROR_FRAME_FLAG =    0x20000000;
    const uint32_t RTR_FRAME_FLAG =      0x40000000;
    const uint32_t EXTENDED_FRAME_FLAG = 0x80000000;

    typedef enum
    {
        DATA_FRAME,
        ERROR_FRAME
    } FRAME_TYPE;

    typedef enum
    {
        STANDARD_FRAME,
        EXTENDED_FRAME
    } FRAME_FORMAT;

    typedef std::vector<uint8_t> CAN_DATA;

    typedef uint32_t CAN_ID;

    typedef std::vector< uint32_t > FILTER_LIST;
    
    NsocketCan(const std::string device = "can0", const bool rxAllCanInterfaces = false );

	virtual ~NsocketCan();

    void setRxFilter( const FILTER_LIST filterList );

    void tx(    const CAN_ID    canId,
                const CAN_DATA  data );

    void tx(    const CAN_DATA      data,
                const CAN_ID        canId,
                const FRAME_FORMAT  format = EXTENDED_FRAME,
                const FRAME_TYPE    type = DATA_FRAME,
                const bool          remoteTransmissionRequest = false );

    CAN_ID rx( CAN_DATA& data );

    void extractFlags(  const CAN_ID    canIdMsg,
                        uint32_t&       canId,
                        bool&           errorFrame,
                        bool&           remoteTransmissionRequest,
                        bool&           extendedFrame );

    void setMessageLoopback( const bool loopbackEnabled );

    void setRxOwnMessages( const bool rxOwnMessages );

private:
    std::string m_device;
    int m_socket;
};
