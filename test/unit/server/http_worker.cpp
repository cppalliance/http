//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/http_worker.hpp>

#include "test_worker.hpp"

namespace boost {
namespace http {

struct http_worker_test
    : test_worker
{
    void
    testBasicRoute()
    {
        // Route returns 200 with body
        check(GET, "/hello",
            h_text("world"),
            status::ok, "world");

        // Unmatched path returns 404
        {
            test_router r;
            r.add(GET, "/hello", h_text("x"));
            check(r, GET, "/missing",
                status::not_found);
        }

        // Wrong method returns 405
        {
            test_router r;
            r.add(GET, "/hello", h_text("x"));
            check(r, POST, "/hello",
                status::method_not_allowed);
        }

        // Custom status code
        check(GET, "/nc",
            h_stat(status::no_content),
            status::no_content);

        // Multiple routes, correct dispatch
        {
            test_router r;
            r.add(GET, "/a", h_text("A"));
            r.add(GET, "/b", h_text("B"));
            check(r, GET, "/b",
                status::ok, "B");
        }

        // HTML body sniffs content-type
        {
            test_router r;
            r.add(GET, "/page",
                h_text("<h1>hi</h1>"));
            auto res = exchange(r, GET, "/page");
            BOOST_TEST(res.status() == status::ok);
            BOOST_TEST(res.body == "<h1>hi</h1>");
            BOOST_TEST(res.res.exists(
                field::content_type));
        }

        // Custom content-type
        check(GET, "/json",
            h_typed("application/json",
                R"({"ok":true})"),
            status::ok, R"({"ok":true})");
    }

    void
    run()
    {
        testBasicRoute();
    }
};

TEST_SUITE(
    http_worker_test,
    "boost.http.http_worker");

} // http
} // boost
