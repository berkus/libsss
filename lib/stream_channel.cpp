#include <algorithm>
#include "stream_channel.h"
#include "private/stream_peer.h"
#include "logging.h"

using namespace std;

namespace ssu {

/**
 * Maximum number of in-use SIDs to skip while trying to allocate one,
 * before we just give up and detach an existing one in this range.
 */
constexpr int max_sid_skip = 16;

// Stream ID 0 always refers to the root stream.
constexpr stream_protocol::stream_id_t root_sid = 0x0000;

stream_channel::stream_channel(shared_ptr<host> host, stream_peer* peer, const peer_id& id)
    : channel(host)
    , peer_(peer)
    , root_(make_shared<base_stream>(host, id, nullptr))
    , transmit_sid_counter_{1}
    , transmit_sid_acked_{0}
    , received_sid_counter_{0}
{
    root_->state_ = base_stream::state::connected;

    // Pre-attach the root stream to this channel in both directions.
    root_->tx_attachments_[0].set_attaching(this, root_sid);
    root_->tx_attachments_[0].set_active(1);
    root_->tx_current_attachment_ = &root_->tx_attachments_[0];

    root_->rx_attachments_[0].set_active(this, root_sid, 1);

    // Listen on the root stream for top-level application streams
    root_->listen(stream::listen_mode::unlimited);

    on_ready_transmit.connect(boost::bind(&stream_channel::got_ready_transmit, this));
    on_link_status_changed.connect(boost::bind(&stream_channel::got_link_status_changed, this, _1));
}

stream_channel::~stream_channel()
{}

void stream_channel::got_ready_transmit()
{
    if (sending_streams_.empty())
        return;

    logger::debug() << "stream_channel: ready to transmit";

    // Round-robin between our streams for now.
    do {
        // Grab the next stream in line to transmit
        base_stream *stream = sending_streams_.front();
        sending_streams_.pop();

        // Allow it to transmit one packet.
        // It will add itself back onto sending_streams_ if it has more.
        stream->transmit_on(this);

    } while (!sending_streams_.empty() and may_transmit());
}

void stream_channel::got_link_status_changed(link::status new_status)
{
    logger::debug() << "stream_channel: link status changed, new status " << int(new_status);
}

stream_protocol::counter_t stream_channel::allocate_transmit_sid()
{
    counter_t sid = transmit_sid_counter_;
    if (transmit_sids_.find(sid) != transmit_sids_.end())
    {
        int maxsearch = 0x7ff0 - (transmit_sid_counter_ - transmit_sid_acked_);
        maxsearch = min(maxsearch, max_sid_skip);
        do {
            if (maxsearch-- <= 0) {
                logger::fatal() << "allocate_transmit_sid: no free SIDs";
                // @fixme: do the actual detach
            }
        } while (transmit_sids_.find(++sid) != transmit_sids_.end());
    }
    assert(sid >= transmit_sid_counter_);
    transmit_sid_counter_ = sid + 1;

    return sid;
}

void stream_channel::start(bool initiate)
{
    logger::debug() << "stream_channel: start " << (initiate ? "(initiator)" : "(responder)");
    super::start(initiate);
    assert(is_active());

    // Set the root stream's USID based on our channel ID
    root_->usid_.half_channel_id_ = initiate ? tx_channel_id() : rx_channel_id();
    root_->usid_.counter_ = 0;
    assert(!root_->usid_.is_empty());

    // If our target doesn't yet have an active flow, use this one.
    // This way we either an incoming or outgoing flow can be a primary.
    target_peer()->channel_started(this);
}

void stream_channel::stop()
{
    logger::debug() << "stream_channel: stop";
    super::stop();
}

bool stream_channel::transmit_ack(byte_array &pkt, uint64_t ackseq, unsigned ackct)
{
    logger::debug() << "stream_channel: transmit_ack";
    return false;
}

void stream_channel::acknowledged(uint64_t txseq, int npackets, uint64_t rxackseq)
{
    logger::debug() << "stream_channel: acknowledged";
}

void stream_channel::missed(uint64_t txseq, int npackets)
{
    logger::debug() << "stream_channel: missed";
}

void stream_channel::expire(uint64_t txseq, int npackets)
{
    logger::debug() << "stream_channel: expire";
}

bool stream_channel::channel_receive(uint64_t pktseq, byte_array &pkt)
{
    logger::debug() << "stream_channel: channel_receive";
    return false;
}

} // ssu namespace
