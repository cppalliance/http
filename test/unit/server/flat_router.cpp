//
// Copyright (c) 2026 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/flat_router.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct flat_router_test
{
    void run()
    {
        // Header compilation test only.
        // Functional tests are in router.cpp.
    }
};

TEST_SUITE(
    flat_router_test,
    "boost.http.server.flat_router");

} // http
} // boost
