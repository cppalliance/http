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

// Full functional tests are in beast2/test/unit/server/router.cpp

#include <boost/http/server/basic_router.hpp>
#include <boost/http/server/router.hpp>

#include <boost/capy/test/run_blocking.hpp>
#include "test_suite.hpp"

namespace boost {
namespace http {

struct flat_router_test
{
    using params = route_params_base;
    using test_router = basic_router<params>;

    void testCopyConstruction()
    {
        auto counter = std::make_shared<int>(0);
        test_router r;
        r.all("/", [counter](params&) -> route_task
        {
            ++(*counter);
            co_return route_result{};
        });

        flat_router fr1(std::move(r));
        flat_router fr2(fr1);

        params req;
        capy::test::run_blocking()(fr1.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 1);

        capy::test::run_blocking()(fr2.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 2);
    }

    void testCopyAssignment()
    {
        auto counter = std::make_shared<int>(0);
        test_router r;
        r.all("/", [counter](params&) -> route_task
        {
            ++(*counter);
            co_return route_result{};
        });

        flat_router fr1(std::move(r));

        test_router r2;
        r2.all("/", [](params&) -> route_task
        {
            co_return route_result{};
        });
        flat_router fr2(std::move(r2));

        fr2 = fr1;

        params req;
        capy::test::run_blocking()(fr1.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 1);

        capy::test::run_blocking()(fr2.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 2);
    }

    void run()
    {
        testCopyConstruction();
        testCopyAssignment();
    }
};

TEST_SUITE(
    flat_router_test,
    "boost.http.server.flat_router");

} // http
} // boost
