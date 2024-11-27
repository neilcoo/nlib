// nzmq.cxx by Neil Cooper. See nzmq.h for documentation
#include "nzmq.h"

#include "nerror.h"

using namespace std;


static zmq::socket_type getZmqType( const Nzmq::MODE mode )
{
    bool valid = false;
    zmq::socket_type type = zmq::socket_type::req;

    switch( mode )
        {
        case Nzmq::MODE::REQ:
            valid = true;
            type = zmq::socket_type::req;
            break;

        case Nzmq::MODE::REP:
            valid = true;
            type = zmq::socket_type::rep;
            break;

        case Nzmq::MODE::DEALER:
            valid = true;
            type = zmq::socket_type::dealer;
            break;

        case Nzmq::MODE::ROUTER:
            valid = true;
            type = zmq::socket_type::router;
            break;

        case Nzmq::MODE::PUB:
            valid = true;
            type = zmq::socket_type::pub;
            break;

        case Nzmq::MODE::SUB:
            valid = true;
            type = zmq::socket_type::sub;
            break;

        case Nzmq::MODE::XPUB:
            valid = true;
            type = zmq::socket_type::xpub;
            break;

        case Nzmq::MODE::XSUB:
            valid = true;
            type = zmq::socket_type::xsub;
            break;

        case Nzmq::MODE::PUSH:
            valid = true;
            type = zmq::socket_type::push;
            break;

        case Nzmq::MODE::PULL:
            valid = true;
            type = zmq::socket_type::pull;
            break;

#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 2, 0)

        case Nzmq::MODE::SERVER:
            valid = true;
            type = zmq::socket_type::server;
            break;

        case Nzmq::MODE::CLIENT:
            valid = true;
            type = zmq::socket_type::client;
            break;

        case Nzmq::MODE::RADIO:
            valid = true;
            type = zmq::socket_type::radio;
            break;

        case Nzmq::MODE::DISH:
            valid = true;
            type = zmq::socket_type::dish;
            break;

        case Nzmq::MODE::GATHER:
            valid = true;
            type = zmq::socket_type::gather;
            break;

        case Nzmq::MODE::SCATTER:
            valid = true;
            type = zmq::socket_type::scatter;
            break;

        case Nzmq::MODE::DGRAM:
            valid = true;
            type = zmq::socket_type::dgram;
            break;
#endif
#if defined(ZMQ_BUILD_DRAFT_API) && ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 3, 3)
        case Nzmq::MODE::PEER:
            valid = true;
            type = zmq::socket_type::peer;
            break;

        case Nzmq::MODE::CHANNEL:
            valid = true;
            type = zmq::socket_type::channel;
            break;
#endif
#if ZMQ_VERSION_MAJOR >= 4
        case Nzmq::MODE::STREAM:
            valid = true;
            type = zmq::socket_type::stream;
            break;
#endif
        case Nzmq::MODE::PAIR:
            valid = true;
            type = zmq::socket_type::pair;
            break;

        default:
            ERROR("Nzmq: Unknown mode value.");
    }

    if ( !valid )
        ERROR("Nzmq: Mode not supported by installed version of ZeroMQ.");

    return type;
}


Nzmq::Nzmq( const std::string destination,
            const MODE mode,
            const TYPE type,
            const string routingId,
            const int ioThreadCount ) :
    m_context(nullptr),
    m_socket(nullptr)
{
    try
        {
        m_context = new zmq::context_t( ioThreadCount );
        }
    catch( zmq::error_t e )
        {
        EERROR( "Nzmq: Can't create 0MQ context: ", e.what() );
        }

    try
        {
        m_socket = new zmq::socket_t( *m_context, getZmqType( mode ) );
        }
    catch( zmq::error_t e )
        {
        EERROR( "Nzmq: Can't create 0MQ socket: ", e.what() );
        }

    if ( routingId.length() )
        try
            {
#if CPPZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 7, 0)
            m_socket->set( zmq::sockopt::routing_id, routingId.c_str() );
#else
            m_socket->setsockopt( ZMQ_ROUTING_ID, routingId.c_str(), routingId.length() );
#endif
            }
        catch( zmq::error_t e )
            {
            EERROR( "Nzmq: Can't set routing id on socket: ", e.what() );
            }

    // Bind if server connection type, else connect
    if ( type == BIND )
        {
        try
            {
            m_socket->bind( destination );
            }
        catch( zmq::error_t e )
            {
            EERROR( "Nzmq: Can't bind to '", destination, "': ", e.what() );
            }
        }
    else
        {
        try
            {
            m_socket->connect( destination );
            }
        catch( zmq::error_t e )
            {
            EERROR( "Nzmq: Can't connect to '", destination, "': ", e.what() );
            }
         }
}

Nzmq::~Nzmq()
{
    if ( m_socket )
        delete m_socket;
    if ( m_context )
        delete m_context;
}


size_t Nzmq::tx( const std::string msg, const bool dontWait, const bool sendMore )
{
    zmq::send_result_t result;
    size_t msgLength = msg.length();
    zmq::send_flags flags = zmq::send_flags::none |
        ( dontWait ? zmq::send_flags::dontwait : zmq::send_flags::none ) |
        ( sendMore ? zmq::send_flags::sndmore : zmq::send_flags::none );

    zmq::message_t zmsg( msgLength );
    memcpy( zmsg.data(), msg.data(), msgLength );
    try
        {
        result = m_socket->send( zmsg, flags );
        }
    catch( zmq::error_t e )
        {
        EERROR( "Nzmq: Can't send on socket: ", e.what() );
        }

    size_t sentLength = 0;
    if ( result.has_value() )
        sentLength = result.value();

    return sentLength;
}


size_t Nzmq::rx( std::string& msg, const bool dontWait )
{
    zmq::message_t rxMsg;
    zmq::recv_result_t result;
    zmq::recv_flags flags = zmq::recv_flags::none |
        ( dontWait ? zmq::recv_flags::dontwait : zmq::recv_flags::none );

    try
        {
        result = m_socket->recv( rxMsg, zmq::recv_flags::none );
        }
    catch( zmq::error_t e )
        {
        EERROR( "Nzmq: Can't receive on socket: ", e.what() );
        }

    size_t receivedLength = 0;
    if ( result.has_value() )
        receivedLength = result.value();

    msg.assign( static_cast<const char*>(rxMsg.data()), rxMsg.size() );

    return receivedLength;
}




