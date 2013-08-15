//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <cstdint>
#include <type_traits>
#include "byte_array.h"
#include "opaque_endian.h"

// Slightly silly enum-class-to-underlying-type converter.
// TODO: move to support lib.
template <typename E>
typename std::underlying_type<E>::type to_underlying(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

namespace ssu {

/**
 * @internal
 * SSU stream protocol definitions.
 * This class simply provides SSU protcol definition constants
 * for use in the other stream classes.
 */
class stream_protocol
{
public:
    // Control chunk magic value for the structured streams.
    // 0x535355 = 'SSU': 'Structured Streams Unleashed'
    static const uint32_t magic;

    typedef uint64_t counter_t; ///< Counter for SID assignment.

    enum class packet_type : uint8_t {
    	invalid  = 0x0,
    	init     = 0x1,
    	reply    = 0x2,
    	data     = 0x3,
    	datagram = 0x4,
    	ack      = 0x5,
    	reset    = 0x6,
    	attach   = 0x7,
    	detach   = 0x8,
    };

    struct stream_header
    {
        big_uint16_t stream_id;
        packet_type  type;
        uint8_t      window;
    };

    struct init_header : public stream_header
    {
    	big_uint16_t new_stream_id;
    	big_uint16_t tx_seq_no;
    };
    typedef init_header reply_header;
    struct data_header : public stream_header
    {
    	big_uint32_t tx_seq_no;
    };
    typedef stream_header datagram_header;
    typedef stream_header ack_header;
    typedef stream_header reset_header;
    typedef stream_header attach_header;
    typedef stream_header detach_header;

    enum class flags : uint8_t
    {
        // Subtype/flag bits for Init, Reply, and Data packets
        data_close        = 0x1,  ///< End of stream.
        data_message      = 0x2,  ///< End of message.
        data_push         = 0x4,  ///< Push to application.
        data_all          = 0x7,  ///< All signal flags.

        // Flag bits for Datagram packets
        dgram_begin       = 0x2,  ///< First fragment.
        dgram_end         = 0x1,  ///< Last fragment.

        // Flag bits for Attach packets
        attach_init       = 0x8,  ///< Initiate stream.
        attach_slot_mask  = 0x1,  ///< Slot to use.

        // Flag bits for Reset packets
        reset_remote      = 0x1,  ///< SID orientation (set: sent LSID is in remote space)
    };

    /**
     * Type for identifying streams uniquely across channels.
     *
     * XXX should contain a "keying method identifier" of some kind?
     */
    struct unique_stream_id_t
    {
        counter_t counter; ///< Stream counter in channel
        byte_array half_channel_id; ///< Unique channel+direction ID 
                                    ///< ("half-channel id")
    };
};

} // namespace ssu
