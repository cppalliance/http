//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/server/router_types.hpp>
#include <boost/http_proto/server/router.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http_proto {

struct router_types_test
{
    void
    run()
    {
        basic_router<route_params_base> r;
        r.add(http_proto::method::post, "/",
            [](route_params_base& rp) ->
                route_result
            {
                BOOST_TEST(  rp.is_method(http_proto::method::post));
                BOOST_TEST(! rp.is_method(http_proto::method::get));
                BOOST_TEST(! rp.is_method("GET"));
                BOOST_TEST(! rp.is_method("Post"));
                BOOST_TEST(  rp.is_method("POST"));
                return route::send;
            });
        route_params_base rp;
        auto rv = r.dispatch("POST", "/", rp);
        BOOST_TEST(rv = route::send);
    }
};

TEST_SUITE(
    router_types_test,
    "boost.http_proto.server.router_types");

} // http_proto
} // boost
