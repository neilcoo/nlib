#pragma once

//  Nzmq v0.1 by Neil Cooper 22 April 2022
//  Encapsulates a ZeroMQ client socket
//  Depdency of cppzmq (sudo apt install libzmq3-dev)

#include <string>
#include "zmq.hpp"

class Nzmq
{
public:
    typedef enum
        {
        REQ = ZMQ_REQ,
        REP = ZMQ_REP,
        DEALER = ZMQ_DEALER,
        ROUTER = ZMQ_ROUTER,
        PUB = ZMQ_PUB,
        SUB = ZMQ_SUB,
        XPUB = ZMQ_XPUB,
        XSUB = ZMQ_XSUB,
        PUSH = ZMQ_PUSH,
        PULL = ZMQ_PULL,
#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 0)
        SERVER = ZMQ_SERVER,
        CLIENT = ZMQ_CLIENT,
        RADIO = ZMQ_RADIO,
        DISH = ZMQ_DISH,
        GATHER = ZMQ_GATHER,
        SCATTER = ZMQ_SCATTER,
        DGRAM = ZMQ_DGRAM,
#endif
#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 3)
        PEER = ZMQ_PEER,
        CHANNEL = ZMQ_CHANNEL,
#endif
#if ZMQ_VERSION_MAJOR >= 4
        STREAM = ZMQ_STREAM,
#endif
        PAIR = ZMQ_PAIR
        } MODE;

    typedef enum
        {
        BIND,
        CONNECT
        } TYPE;


    Nzmq( const std::string destination, const MODE mode, const TYPE type = CONNECT, const std::string routingId = "", const int ioThreadCount = 1 );
    virtual ~Nzmq();

    size_t tx( const std::string msg, const bool dontWait = false, const bool sendMore = false );
    size_t rx( std::string& msg, const bool dontWait = false );

private:
    zmq::context_t* m_context;
    zmq::socket_t* m_socket;
};

