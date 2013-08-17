//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_TEST_MODULE Test_ssu_host
#include <boost/test/unit_test.hpp>

#include "host.h"

using namespace ssu;

BOOST_AUTO_TEST_CASE(created_host)
{
    std::shared_ptr<host> h(new host);
}
