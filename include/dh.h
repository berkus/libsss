//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "byte_array.h"
#include "timer.h"
#include "negotiation/key_message.h"
#include <openssl/dh.h>

namespace ssu {

class host;
class dh_host_state;

namespace negotiation {

class key_responder;
class key_initiator;

class dh_hostkey_t : public std::enable_shared_from_this<dh_hostkey_t>
{
    friend class ssu::dh_host_state;
    friend class ssu::negotiation::key_responder; // @fixme remove
    friend class ssu::negotiation::key_initiator; // @fixme remove

    std::shared_ptr<ssu::host> host_;    // Host to which this key is attached.
    ssu::async::timer expiration_timer_; // Expiration timer for DH master key.
    negotiation::dh_group_type group_;
    DH *dh_;
    byte_array public_key_; // Host's public key.
    byte_array hmac_secret_key_; // HMAC key for responder's challenge.

    /**
     * Hash table of cached R2 responses made using this key, for replay protection.
     */
    std::map<byte_array, byte_array> r2_cache_;

    dh_hostkey_t(std::shared_ptr<ssu::host> host, negotiation::dh_group_type group, DH *dh);

    /**
     * Expire the key.
     */
    void timeout();

    /**
     * Compute a shared master secret from our private key and other_public_key.
     */
    byte_array calc_key(byte_array const& other_public_key);
};

} // namespace negotiation

class dh_host_state
{
    friend class negotiation::dh_hostkey_t; // @fixme remove

    std::shared_ptr<negotiation::dh_hostkey_t>
    dh_keys_[/*dh_group_max*/3];

    std::shared_ptr<negotiation::dh_hostkey_t>
    _generate_dh_key(negotiation::dh_group_type group, DH *(*groupfunc)());

public:
    dh_host_state();
    virtual ~dh_host_state();

    std::shared_ptr<negotiation::dh_hostkey_t> get_dh_key(negotiation::dh_group_type group);

    virtual std::shared_ptr<host> get_host() = 0;
};

} // namespace ssu
