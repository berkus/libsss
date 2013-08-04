//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <boost/serialization/detail/stack_constructor.hpp>
#include "msgpack.h"

BOOST_CLASS_IMPLEMENTATION(optional<uint32_t>, boost::serialization::object_serializable)

// function specializations must be defined in the appropriate
// namespace - boost::serialization
namespace boost {
namespace serialization {

template<class Archive, class T>
void save(
    Archive & ar, 
    const boost::optional< T > & t, 
    const unsigned int /*version*/
){
    const bool tflag = t.is_initialized();
    msgpack::encode_boolean(ar, tflag);
    if (tflag) {
        ar << *t;
    }
}

template<class Archive, class T>
void load(
    Archive & ar, 
    boost::optional< T > & t, 
    const unsigned int /*version*/
){
    bool tflag = msgpack::decode_boolean(ar);
    if (tflag) {
        detail::stack_construct<Archive, T> aux(ar, 0);
        ar >> aux.reference();
        t.reset(aux.reference());
    }
    else {
        t.reset();
    }
}

template<class Archive, class T>
void serialize(
    Archive & ar, 
    boost::optional< T > & t, 
    const unsigned int version
){
    boost::serialization::split_free(ar, t, version);
}

} // namespace serialization
} // namespace boost
