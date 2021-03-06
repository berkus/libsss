//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2015, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "sss/framing/framing_types.h"
#include "sss/framing/stream_protocol.h"
#include "arsenal/opaque_endian.h"
#include "arsenal/underlying.h"
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/include/equal_to.hpp>

//=================================================================================================
// Framing layer
//=================================================================================================

// namespace sss::framing { // -std=c++1z with SVN clang
namespace sss {
namespace framing {

using packet_flag_field_t = field_flag<uint8_t>;
using version_field_t     = optional_field_specification<uint16_t, field_index<0>, 0_bits_shift>;
using fec_field_t         = optional_field_specification<uint8_t, field_index<0>, 1_bits_shift>;
using packet_size_t = varsize_field_wrapper<packet_sequence_number, uint64_t>;
using packet_field_t =
    varsize_field_specification<packet_size_t, field_index<0>, 2_bits_mask, 2_bits_shift>;

} // framing namespace
} // sss namespace

// clang-format off
BOOST_FUSION_DEFINE_STRUCT( // done: r
    (sss)(framing), packet_header,
    (sss::framing::packet_flag_field_t, flags) // 000fssgv
    (sss::framing::version_field_t, version)
    (sss::framing::fec_field_t, fec_group)
    (sss::framing::packet_field_t, packet_sequence)
);

//-------------------------------------------------------------------------------------------------
// STREAM frame
//-------------------------------------------------------------------------------------------------

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), packet_stream_offset,
    (nothing_t, size0)
    (uint16_t, size2)
    (sss::framing::uint24_t, size3)
    (uint32_t, size4)
    (sss::framing::uint40_t, size5)
    (sss::framing::uint48_t, size6)
    (sss::framing::uint56_t, size7)
    (uint64_t, size8)
);
// clang-format on

namespace sss {
namespace framing {

using empty_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::EMPTY)>;
using stream_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::STREAM)>;
using ack_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::ACK)>;
using padding_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::PADDING)>;
using decongestion_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::DECONGESTION)>;
using detach_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::DETACH)>;
using reset_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::RESET)>;
using close_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::CLOSE)>;
using settings_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::SETTINGS)>;
using priority_frame_type_t =
    std::integral_constant<uint8_t, to_underlying(stream_protocol::frame_type::PRIORITY)>;
using max_frame_count_t = std::integral_constant<uint8_t, 10>;

using stream_flags_field_t  = field_flag<uint8_t>;
using optional_parent_sid_t = optional_field_specification<uint32_t, field_index<1>, 6_bits_shift>;
using optional_usid_t       = optional_field_specification<usid_t, field_index<1>, 5_bits_shift>;
using optional_data_t       = optional_field_specification<std::string, field_index<1>, 1_bits_shift>;
using stream_offset_t = varsize_field_wrapper<packet_stream_offset, uint64_t>;
using packet_stream_offset_t =
    varsize_field_specification<stream_offset_t, field_index<1>, 3_bits_mask, 2_bits_shift>;

} // framing namespace
} // sss namespace

// clang-format off
BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), empty_frame_header,
    (sss::framing::empty_frame_type_t, type)
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), stream_frame_header,
    (sss::framing::stream_frame_type_t, type)
    (sss::framing::stream_flags_field_t, flags)
    (uint32_t, stream_id)
    (sss::framing::optional_parent_sid_t, parent_stream_id)
    (sss::framing::optional_usid_t, usid)
    (sss::framing::packet_stream_offset_t, stream_offset)
    (sss::framing::optional_data_t, frame)
    // ^^ ext length spec through data_length member
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), ack_frame_header,
    (sss::framing::ack_frame_type_t, type)
    (big_uint8_t, sent_entropy)
    (big_uint8_t, received_entropy)
    (big_uint8_t, missing_packets)
    (big_uint64_t, least_unacked_packet)
    (big_uint64_t, largest_observed_packet)
    (big_uint32_t, largest_observed_delta_time)
    (std::vector<uint64_t>, nacks)
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), padding_frame_header,
    (sss::framing::padding_frame_type_t, type)
    (big_uint16_t, length)
    (rest_t, frame) // [length] padding data - @todo only until end of the frame! ext length spec...
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), decongestion_frame_header,
    (sss::framing::decongestion_frame_type_t, type)
    (big_uint8_t, subtype)
    //(rest_t, data) // @todo need to change this field to more clear type.
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), detach_frame_header,
    (sss::framing::detach_frame_type_t, type)
    (big_uint32_t, lsid)
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), reset_frame_header,
    (sss::framing::reset_frame_type_t, type)
    (uint32_t, lsid)
    (big_uint32_t, error_code)
    (std::string, reason_phrase)
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), close_frame_header,
    (sss::framing::close_frame_type_t, type)
    (big_uint32_t, error_code)
    (std::string, reason_phrase)
    (sss::framing::ack_frame_header, final_ack_frame)
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), settings_frame_header,
    (sss::framing::settings_frame_type_t, type)
    (big_uint16_t, number_of_settings)
// @todo revisit
//    (sss::framing::frame_data_t, settings_tag)
);

BOOST_FUSION_DEFINE_STRUCT(
    (sss)(framing), priority_frame_header,
    (sss::framing::priority_frame_type_t, type)
    (uint32_t, lsid)
    (uint32_t, priority_value)
);
// clang-format on

namespace sss {
namespace framing {

inline bool
operator==(empty_frame_header const& f, empty_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(stream_frame_header const& f, stream_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(ack_frame_header const& f, ack_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(padding_frame_header const& f, padding_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(decongestion_frame_header const& f, decongestion_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(detach_frame_header const& f, detach_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(reset_frame_header const& f, reset_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(close_frame_header const& f, close_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(settings_frame_header const& f, settings_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

inline bool
operator==(priority_frame_header const& f, priority_frame_header const& s)
{
    return boost::fusion::equal_to(f, s);
}

} // framing namespace
} // sss namespace
