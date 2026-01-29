//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/router_types.hpp>
#include <boost/http/server/basic_router.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct router_types_test
{
    void
    run()
    {
        // Test route_result construction and accessors
        {
            route_result r1(route_done);
            BOOST_TEST(r1.what() == route_what::done);

            route_result r2(route_next);
            BOOST_TEST(r2.what() == route_what::next);

            route_result r3(route_next_route);
            BOOST_TEST(r3.what() == route_what::next_route);

            route_result r4(route_close);
            BOOST_TEST(r4.what() == route_what::close);
        }
    }
};

TEST_SUITE(
    router_types_test,
    "boost.http.server.router_types");

} // http
} // boost
