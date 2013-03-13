#pragma once

#include <map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/signals2/signal.hpp>
#include "byte_array.h"

namespace ssu {

class link;

typedef uint32_t magic_t; ///< Protocol magic marker, must have 0x00 as the highest byte (channel number).
typedef uint8_t channel_number;
/**
 * Currently only UDP endpoints/sockets are supported.
 * System implementation might also have to work over
 * IP or even Ethernet endpoints - this will require
 * architectural change.
 */
typedef boost::asio::ip::udp::endpoint endpoint;

/**
 * Add an association with particular link to the endpoint.
 */
class link_endpoint : public endpoint
{
    std::weak_ptr<link> link_;

    // Send a message to this endpoint on this socket
    bool send(const char *data, int size) const;
    inline bool send(const byte_array& msg) const {
        return send(msg.const_data(), msg.size());
    }
};

/**
 * Base class for socket-based channels,
 * for dispatching received packets based on endpoint and channel number.
 * May be used as an abstract base by overriding the receive() method,
 * or used as a concrete class by connecting to the received() signal.
 */
class link_channel
{
public: // Provide access to signal types for clients
    typedef boost::signals2::signal<void (byte_array&, const link_endpoint&)> on_received;
    typedef boost::signals2::signal<void ()> on_ready_transmit;

    on_received received;
    // Signalled when flow/congestion control may allow new transmission
    on_ready_transmit ready_transmit;

private:
    std::weak_ptr<link> link_; ///< Link we're currently bound to, if any.
    bool active;               ///< True if we're sending and accepting packets.
};

/**
 * Control protocol receiver.
 * Provides support for receiving control messages for
 * registered protocol types.
 */
class link_receiver
{

};

/**
 * This class encapsulates link-related part of host state.
 */
class link_host_state
{
    virtual link* create_link();
    virtual link_receiver* receiver(magic_t magic);
};

/**
 * Abstract base class for entity connecting two endpoints using some network.
 * Link manages connection lifetime and maintains the connection status info.
 * For connected links there may be a number of channels established using their own keying schemes.
 * Link orchestrates initiation of key exchanges and scheme setup.
 */
class link
{
    link_host_state* const host;
    std::map<std::pair<link_endpoint, channel_number>, link_channel*> channels;
    bool active_;

public:
    // ssu expresses current link status as one of three states:
    // - up: apparently alive, all's well as far as we know.
    // - stalled: briefly lost connectivity, but may be temporary.
    // - down: definitely appears to be down.
    enum class status {
        down,
        stalled,
        up
    };

    link(link_host_state* h) : host(h) {}
    ~link();

    bool send(const endpoint&ep, const char* data, size_t size) { return false; }

protected:
    /**
     * Implementation subclass calls this method with received packets.
     * @param msg the packet received.
     * @param src the source from which the packet arrived.
     */
    void receive(byte_array& msg, const link_endpoint& src);
};

/**
 * Class for UDP connection between two endpoints.
 * Multiplexes between channel-setup/key exchange traffic (which goes to key.cpp)
 * and per-channel data traffic (which goes to channel.cpp).
 */
class udp_link : public link
{
    boost::asio::ip::udp::socket udp_socket;
    byte_array received_buffer;
    endpoint received_from;

public:
    bool bind(const endpoint& ep);
    /// Send a packet on this UDP socket.
    bool send(const endpoint& ep, const char *data, int size);
    inline bool send(const endpoint& ep, const byte_array& msg) {
        return send(ep, msg.const_data(), msg.size());
    }

    /// Return all known local endpoints referring to this link.
    std::vector<endpoint> local_endpoints();

private:
    void prepare_async_receive();
    void udp_ready_read(const boost::system::error_code& error, std::size_t bytes_transferred);
};

} // namespace ssu
