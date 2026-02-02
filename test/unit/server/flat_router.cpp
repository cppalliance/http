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

    void testDefaultConstruction()
    {
        auto counter = std::make_shared<int>(0);
        test_router r;
        r.all("/", [counter](params&) -> route_task
        {
            ++(*counter);
            co_return route_result{};
        });

        flat_router fr1;  // default construct
        flat_router fr2(std::move(r));
        fr1 = fr2;  // assign to default-constructed

        params req;
        capy::test::run_blocking()(fr1.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 1);
    }

    void testOptionsHandler()
    {
        std::string captured_allow;
        test_router r;
        r.add(http::method::get, "/api/users", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.add(http::method::post, "/api/users", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.set_options_handler(
            [&captured_allow](params&, std::string_view allow) -> route_task
            {
                captured_allow = allow;
                co_return route_done;
            });

        flat_router fr(std::move(r));

        params req;
        capy::test::run_blocking()(fr.dispatch(
            http::method::options, urls::url_view("/api/users"), req));
        BOOST_TEST(captured_allow.find("GET") != std::string::npos);
        BOOST_TEST(captured_allow.find("POST") != std::string::npos);
    }

    void testExplicitOptionsPriority()
    {
        bool explicit_called = false;
        bool fallback_called = false;

        test_router r;
        r.add(http::method::get, "/test", [](params&) -> route_task
        {
            co_return route_done;
        });
        // Explicit OPTIONS handler
        r.add(http::method::options, "/test",
            [&explicit_called](params&) -> route_task
            {
                explicit_called = true;
                co_return route_done;
            });
        // Fallback handler
        r.set_options_handler(
            [&fallback_called](params&, std::string_view) -> route_task
            {
                fallback_called = true;
                co_return route_done;
            });

        flat_router fr(std::move(r));

        params req;
        capy::test::run_blocking()(fr.dispatch(
            http::method::options, urls::url_view("/test"), req));
        BOOST_TEST(explicit_called);
        BOOST_TEST(!fallback_called);
    }

    void testAllMethodsHandler()
    {
        std::string captured_allow;
        test_router r;
        // Use route().all() but have handler return route_next
        // so OPTIONS fallback can run
        r.route("/wildcard").all([](params&) -> route_task
        {
            co_return route_next;
        });
        r.set_options_handler(
            [&captured_allow](params&, std::string_view allow) -> route_task
            {
                captured_allow = allow;
                co_return route_done;
            });

        flat_router fr(std::move(r));

        params req;
        capy::test::run_blocking()(fr.dispatch(
            http::method::options, urls::url_view("/wildcard"), req));
        // .all() should produce a full Allow header
        BOOST_TEST(captured_allow.find("GET") != std::string::npos);
        BOOST_TEST(captured_allow.find("POST") != std::string::npos);
        BOOST_TEST(captured_allow.find("DELETE") != std::string::npos);
    }

    void testOptionsStarGlobal()
    {
        std::string captured_allow;
        test_router r;
        r.add(http::method::get, "/a", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.add(http::method::post, "/b", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.add(http::method::put, "/c", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.set_options_handler(
            [&captured_allow](params&, std::string_view allow) -> route_task
            {
                captured_allow = allow;
                co_return route_done;
            });

        flat_router fr(std::move(r));

        params req;
        capy::test::run_blocking()(fr.dispatch(
            http::method::options, urls::url_view("*"), req));
        // Should contain all registered methods
        BOOST_TEST(captured_allow.find("GET") != std::string::npos);
        BOOST_TEST(captured_allow.find("POST") != std::string::npos);
        BOOST_TEST(captured_allow.find("PUT") != std::string::npos);
    }

    void run()
    {
        testCopyConstruction();
        testCopyAssignment();
        testDefaultConstruction();
        testOptionsHandler();
        testExplicitOptionsPriority();
        testAllMethodsHandler();
        testOptionsStarGlobal();
    }
};

TEST_SUITE(
    flat_router_test,
    "boost.http.server.flat_router");

} // http
} // boost
