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
    testRouteOutcomes()
    {
        // route_next: handler declines, path has
        // a route so router returns 405
        check(GET, "/x", h_next,
            status::method_not_allowed);

        // route_next_route: skip route, path has
        // a route so router returns 405
        check(GET, "/x", h_next_route,
            status::method_not_allowed);

        // route_next via middleware -> 404
        {
            test_router r;
            r.use(h_next);
            check(r, GET, "/x",
                status::not_found);
        }

        // route_close: session ends, no response
        {
            test_router r;
            r.add(GET, "/bye", h_close);
            auto raw = exchange_raw(r, GET, "/bye");
            BOOST_TEST(raw.empty());
        }

        // route_error: handler fails -> 500
        check(GET, "/err",
            h_error(http::error::bad_content_length),
            status::internal_server_error);

        // Empty router -> 404
        {
            test_router r;
            check(r, GET, "/anything",
                status::not_found);
        }

        // route_next falls through to second handler
        {
            test_router r;
            r.route("/x")
                .add(GET, h_next)
                .add(GET, h_text("ok"));
            check(r, GET, "/x",
                status::ok, "ok");
        }

        // route_next_route skips entire first route
        {
            test_router r;
            r.add(GET, "/x", h_next_route);
            r.add(GET, "/x", h_text("second"));
            check(r, GET, "/x",
                status::ok, "second");
        }
    }

    void
    run()
    {
        testBasicRoute();
        testRouteOutcomes();
    }
};

TEST_SUITE(
    http_worker_test,
    "boost.http.http_worker");

} // http
} // boost
