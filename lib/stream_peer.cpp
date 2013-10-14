#include "private/stream_peer.h"
#include "stream_channel.h"
#include "algorithm.h"

using namespace std;

namespace ssu {
// namespace internal {

constexpr int DEFAULT_PORT = 9660; /// @todo

constexpr int stream_peer::stall_warnings_max;

const async::timer::duration_type stream_peer::connect_retry_period = boost::posix_time::minutes(1);

stream_peer::stream_peer(shared_ptr<host> const& host, peer_id const& remote_id, stream_peer::private_tag)
    : host_(host)
    , remote_id_(remote_id)
    , reconnect_timer_(host.get())
{
    assert(!remote_id.is_empty());

    // If the remote_id is just an encapsulated IP endpoint,
    // then also use it as a destination address hint.
    identity ident(remote_id_.id());
    if (ident.is_ip_key_scheme()) {
        endpoint ep(ident.get_endpoint());
        if (ep.port() == 0) {
            ep.port(DEFAULT_PORT);
        }
        locations_.insert(ep);
    }

    reconnect_timer_.on_timeout.connect([this](bool failed){ retry_timeout(); });
}

stream_peer::~stream_peer()
{
    logger::debug() << this << " ~stream_peer";
    // Clear the state of all streams associated with this peer.
    for (auto v : all_streams_)
    {
        v->clear();
    }
    assert(all_streams_.empty());
    assert(usid_streams_.empty());
}

void stream_peer::connect_channel()
{
    assert(!remote_id_.is_empty());

    if (primary_channel_ and primary_channel_->link_status() == link::status::up)
        return; // Already have a working channel; don't need another yet.

    // @todo Need a way to determine if streams need to send. If no streams waiting to send
    // on this channel, don't even bother.
    //
    //if (receivers(SIGNAL(flowConnected())) == 0) return;

    logger::debug() << "Trying to connect channel with peer " << remote_id_;

    // @todo Ask routing to figure out other possible endpoints for this peer.

    // // Send a lookup request to each known registration server.
    // foreach (RegClient *rc, h->regClients()) {
    //     if (!rc->registered())
    //         continue;   // Can't poll an inactive regserver
    //     if (lookups.contains(rc))
    //         continue;   // Already polling this regserver

    //     // Make sure we're hooked up to this client's signals
    //     conncli(rc);

    //     // Start the lookup, with hole punching
    //     lookups.insert(rc);
    //     rc->lookup(remote_id_, true);
    // }

    // Initiate key exchange attempts to all already-known endpoints
    // using each of the network links we have available.
    for (auto link : host_->active_links())
    {
        for (auto endpoint : locations_)
        {
            initiate_key_exchange(link, endpoint);
        }
    }

    // Keep firing off connection attempts periodically
    reconnect_timer_.start(connect_retry_period);
}

// --- Some routing stuff still unfinished: ---
//
// void StreamPeer::conncli(RegClient *rc)
// {
//     if (connrcs.contains(rc))
//         return;
//     connrcs.insert(rc);

//     // Listen for the lookup response
//     connect(rc, SIGNAL(lookupDone(const SST::PeerId&,
//             const Endpoint &, const RegInfo &)),
//         this, SLOT(lookupDone(const SST::PeerId&,
//             const Endpoint &, const RegInfo &)));

//     // Also make sure we hear if this regclient disappears
//     connect(rc, SIGNAL(destroyed(QObject*)),
//         this, SLOT(regClientDestroyed(QObject*)));
// }

// void StreamPeer::lookupDone(const SST::PeerId &id, const Endpoint &loc, const RegInfo &info)
// {
//     if (id != this->id) {
//         qDebug() << this << "got lookupDone for wrong id" << id << "expecting" << this->id << "(harmless, ignored)";
//         return; // ignore responses for other lookup requests
//     }

//     // Mark this outstanding lookup as completed.
//     RegClient *rc = (RegClient*)sender();
//     if (!lookups.contains(rc)) {
//         qDebug() << "StreamPeer: unexpected lookupDone signal";
//         return; // ignore duplicates caused by concurrent requests
//     }
//     lookups.remove(rc);

//     // If the lookup failed, notify waiting streams as appropriate.
//     if (loc.isNull()) {
//         qDebug() << this << "Lookup on" << id << "failed";
//         if (!lookups.isEmpty() || !initors.isEmpty())
//             return;     // There's still hope
//         return flowFailed();
//     }

//     qDebug() << "StreamResponder::lookupDone: primary" << loc << "num secondaries" << info.endpoints().size();

//     // Add the endpoint information we've received to our address list,
//     // and initiate flow setup attempts to those endpoints.
//     foundEndpoint(loc);
//     foreach (const Endpoint &ep, info.endpoints())
//         foundEndpoint(ep);
// }

// void StreamPeer::regClientDestroyed(QObject *obj)
// {
//     qDebug() << "StreamPeer: RegClient destroyed before lookupDone";

//     RegClient *rc = (RegClient*)obj;
//     lookups.remove(rc);
//     connrcs.remove(rc);

//     // If there are no more RegClients available at all,
//     // notify waiting streams of connection failure
//     // next time we get back to the main loop.
//     if (lookups.isEmpty() && initors.isEmpty())
//         recontimer.start(0);
// }

void stream_peer::retry_timeout()
{
    // If we actually have an active flow now, do nothing.
    if (primary_channel_ and primary_channel_->link_status() == link::status::up)
        return;

    // Notify any waiting streams of failure
    if (lookups_.empty() and key_exchanges_initiated_.empty())
        on_channel_failed();

    // If there are (still) any waiting streams, fire off a new batch of connection attempts.
    connect_channel();
}

void stream_peer::initiate_key_exchange(link* l, const endpoint& ep)
{
    assert(ep != endpoint());

    // No need to initiate new channels if we already have a working one.
    if (primary_channel_ and primary_channel_->link_status() == link::status::up)
        return;

    // Don't simultaneously initiate multiple channels to the same endpoint.
    // @todo Eventually multipath support is needed.
    link_endpoint lep(l, ep);
    if (contains(key_exchanges_initiated_, lep))
    {
        logger::debug() << "Already attempting connection to " << ep;
        return;
    }

    logger::debug() << "Initiating key exchange from link " << l << " to remote endpoint " << ep;

    // Make sure our stream_responder exists to receive and dispatch incoming
    // key exchange control packets.
    host_->instantiate_stream_responder();

    // Create and bind a new channel.
    channel* chan = new stream_channel(host_, this, remote_id_);
    if (!chan->bind(l, ep)) {
        logger::warning() << "Could not bind new channel to target " << ep;
        delete chan;
        return on_channel_failed();
    }

    // Start the key exchange process for the channel.
    shared_ptr<negotiation::key_initiator> init =
        make_shared<negotiation::key_initiator>(chan, magic_id, remote_id_);

    init->on_completed.connect([this](std::shared_ptr<negotiation::key_initiator> ki, bool success)
    {
        completed(ki, success);
    });

    key_exchanges_initiated_.insert(make_pair(lep, init));
    init->exchange_keys();
}

void stream_peer::channel_started(stream_channel* channel)
{
    logger::debug() << "Stream peer - channel " << channel << " started";

    assert(channel->is_active());
    assert(channel->target_peer() == this);
    assert(channel->link_status() == link::status::up);

    if (primary_channel_)
    {
        // If we already have a working primary channel, we don't need a new one.
        if (primary_channel_->link_status() == link::status::up)
            return; 

        // But if the current primary is on the blink, replace it.
        clear_primary_channel();
    }

    logger::debug() << "Stream peer - new primary channel " << channel;

    // Use this channel as our primary channel for this target.
    primary_channel_ = channel;
    stall_warnings_ = 0;

    // Watch the link status of our primary channel, so we can try to replace it if it fails.
    primary_channel_->on_link_status_changed.connect([this](link::status new_status) {
        primary_status_changed(new_status);
    });

    // Notify all waiting streams
    on_channel_connected();
    on_link_status_changed(link::status::up);
}

void stream_peer::clear_primary_channel()
{
    if (!primary_channel_)
        return;

    auto old_primary = primary_channel_;
    primary_channel_ = nullptr;

    // Avoid getting further primary link status notifications from it
    old_primary->on_link_status_changed.disconnect(
        boost::bind(&stream_peer::primary_status_changed, this, _1));

    // Clear all transmit-attachments
    // and return outstanding packets to the streams they came from.
    old_primary->detach_all();
}

void stream_peer::add_location_hint(endpoint const& hint)
{
    assert(!remote_id_.is_empty());
    // assert(!hint.empty());

    if (contains(locations_, hint))
        return; // We already know; sit down...

    logger::debug() << "Found endpoint " << hint << " for target " << remote_id_;

    // Add this endpoint to our set
    locations_.insert(hint);

    // Attempt a connection to this endpoint
    for (auto link : host_->active_links()) {
        initiate_key_exchange(link, hint);
    }
}

void stream_peer::completed(std::shared_ptr<negotiation::key_initiator> ki, bool success)
{
    assert(ki and ki->is_done());

    // Remove and schedule the key initiator for deletion, in case it wasn't removed already
    // (e.g., if key agreement failed).
    // 
    // @todo Delete channel automatically if key_initiator failed...
    link_endpoint lep = ki->remote_endpoint();

    logger::debug() << "Stream peer key exchange for ID " << remote_id_ << " to " << lep
        << " completed " << (success ? "successfully" : "erroneously");

    assert(!contains(key_exchanges_initiated_, lep) or key_exchanges_initiated_[lep] == ki);
    key_exchanges_initiated_.erase(lep);

    ki->cancel();
    ki.reset();

    // If unsuccessful, notify waiting streams.
    if (!success)
    {
        if (lookups_.empty() and key_exchanges_initiated_.empty())
            return on_channel_failed();
        return; // There's still hope
    }

    // We should have an active primary channel at this point,
    // since stream_channel::start() attaches the channel if there isn't one.
    // Note: the reason we don't just set the primary right here
    // is because stream_channel::start() gets called on incoming streams too,
    // so servers don't have to initiate back-channels to their clients.

    // @todo This invariant doesn't hold here, fixme.
    // assert(primary_channel_ and primary_channel_->link_status() == link::status::up);
}

void stream_peer::primary_status_changed(link::status new_status)
{
    assert(primary_channel_);

    if (new_status == link::status::up)
    {
        stall_warnings_ = 0;
        // Now that we (again?) have a working primary channel, cancel and delete all
        // outstanding key_initiators that are still in an early enough stage not
        // to have possibly created receiver state.
        // (If we were to kill a non-early key_initiator, the receiver might pick one
        // of those streams as _its_ primary and be left with a dangling channel!)
        // For Multipath-SSU to work we rather should not destroy them here and set up
        // multiple channels at once.
        std::vector<link_endpoint> delete_later;

        for (auto ki : key_exchanges_initiated_)
        {
            auto initiator = ki.second;
            if (!initiator->is_early())
                continue;   // too late - let it finish
            logger::debug() << "Deleting " << initiator << " for " << remote_id_
                << " to " << initiator->remote_endpoint();

            assert(ki.first == initiator->remote_endpoint());
            delete_later.push_back(ki.first);
            initiator->cancel();
            initiator.reset();//->deleteLater();
        }

        for (auto k : delete_later) {
            key_exchanges_initiated_.erase(k);
        }

        return on_link_status_changed(new_status);
    }

    if (new_status == link::status::stalled)
    {
        if (++stall_warnings_ < stall_warnings_max) {
            logger::warning() << this << " Primary channel stall "
                << stall_warnings_ << " of " << stall_warnings_max;
            return on_link_status_changed(new_status);
        }
    }

    // Primary is at least stalled, perhaps permanently failed -
    // start looking for alternate paths right away for quick response.
    connect_channel();

    // Pass the signal on to all streams connected to this peer.
    on_link_status_changed(new_status);
}

// } // internal namespace
} // ssu namespace
