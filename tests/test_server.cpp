//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_TEST_MODULE Test_sss_server
#include <boost/test/unit_test.hpp>

#include "sss/server.h"

using namespace std;
using namespace sss;

BOOST_AUTO_TEST_CASE(created_server)
{
    shared_ptr<host> h(host::create());
    server s(h);
}

BOOST_AUTO_TEST_CASE(server_listen)
{
    shared_ptr<host> h(host::create());
    server s(h);
    BOOST_CHECK(s.listen("test", "Testing", "test", "Test protocol") == true);
}
