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
    template<typename Error>
    void
    check(
        char const* name,
        Error ev)
    {
        auto const ec = make_error_code(ev);
        BOOST_TEST(std::string(ec.category().name()) == name);
        BOOST_TEST(! ec.message().empty());
        BOOST_TEST(
            std::addressof(ec.category()) ==
            std::addressof(make_error_code(ev).category()));
        BOOST_TEST(ec.category().equivalent(
            static_cast<typename std::underlying_type<Error>::type>(ev),
                ec.category().default_error_condition(
                    static_cast<typename std::underlying_type<Error>::type>(ev))));
        BOOST_TEST(ec.category().equivalent(ec,
            static_cast<typename std::underlying_type<Error>::type>(ev)));
    }

    void
    run()
    {
        {
            char const* const n = "boost.http.route";
            check(n, route::close);
            check(n, route::complete);
            check(n, route::suspend);
            check(n, route::next);
            check(n, route::next_route);
            check(n, route::send);
        }

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
