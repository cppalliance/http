//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/router.hpp>
#include <boost/http/server/flat_router.hpp>
#include <boost/http/server/detail/router_base.hpp>

#include <boost/capy/ex/run_sync.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct router_test
{
    using params = route_params_base;
    using test_router = router<params>;

    //--------------------------------------------
    // Simple handlers - no destructor verification
    //--------------------------------------------

    // returns success (non-failing error_code)
    static auto h_send(params&) -> capy::task<route_result>
    { co_return route_result{}; }

    // returns route::next
    static auto h_next(params&) -> capy::task<route_result>
    { co_return route::next; }

    // returns route::next_route
    static auto h_next_route(params&) -> capy::task<route_result>
    { co_return route::next_route; }

    // returns specified error
    static auto h_fail(system::error_code ec)
    {
        return [ec](params&) -> capy::task<route_result>
        { co_return ec; };
    }

    // error handler returns success
    static auto eh_send(system::error_code expect)
    {
        return [expect](params&, system::error_code ec) -> capy::task<route_result>
        { BOOST_TEST(ec == expect); co_return route_result{}; };
    }

    // error handler returns route::next
    static auto eh_next(system::error_code expect)
    {
        return [expect](params&, system::error_code ec) -> capy::task<route_result>
        { BOOST_TEST(ec == expect); co_return route::next; };
    }

    // error handler returns a new error
    static auto eh_return(system::error_code new_ec)
    {
        return [new_ec](params&, system::error_code) -> capy::task<route_result>
        { co_return new_ec; };
    }

    // exception handler returns success - return as lambda for template deduction
    static auto exh_send()
    {
        return [](params&, std::exception_ptr) -> capy::task<route_result>
        { co_return route_result{}; };
    }

    // exception handler returns route::next
    static auto exh_next()
    {
        return [](params&, std::exception_ptr) -> capy::task<route_result>
        { co_return route::next; };
    }

    // throws exception
    static auto h_throw(params&) -> capy::task<route_result>
    { throw std::runtime_error("test"); co_return route::next; }

    // checks path then returns success
    static auto h_path(core::string_view base, core::string_view path)
    {
        return [base, path](params& rp) -> capy::task<route_result>
        {
            BOOST_TEST_EQ(rp.base_path, base);
            BOOST_TEST_EQ(rp.path, path);
            co_return route_result{};
        };
    }

    //--------------------------------------------
    // Helper to run dispatch and check result
    //--------------------------------------------

    static void check(
        test_router& r,
        core::string_view url,
        route_result rv0 = route_result{})
    {
        flat_router fr(std::move(r));
        params req;
        auto rv = capy::run_sync()(fr.dispatch(
            http::method::get, urls::url_view(url), req));
        BOOST_TEST_EQ(rv.message(), rv0.message());
    }

    static void check(
        test_router& r,
        http::method verb,
        core::string_view url,
        route_result rv0 = route_result{})
    {
        flat_router fr(std::move(r));
        params req;
        auto rv = capy::run_sync()(fr.dispatch(
            verb, urls::url_view(url), req));
        BOOST_TEST_EQ(rv.message(), rv0.message());
    }

    static void check(
        test_router& r,
        core::string_view verb,
        core::string_view url,
        route_result rv0 = route_result{})
    {
        flat_router fr(std::move(r));
        params req;
        auto rv = capy::run_sync()(fr.dispatch(
            verb, urls::url_view(url), req));
        BOOST_TEST_EQ(rv.message(), rv0.message());
    }

    //--------------------------------------------
    // Tests
    //--------------------------------------------

    void testUse()
    {
        // pathless middleware
        { test_router r; r.use(h_send); check(r, "/"); }
        { test_router r; r.use(h_next, h_send); check(r, "/"); }
        { test_router r; r.use(h_next); r.use(h_send); check(r, "/"); }

        // path prefix matching
        { test_router r; r.use("/api", h_next); check(r, "/", route::next); }
        { test_router r; r.use("/api", h_send); check(r, "/api"); }
        { test_router r; r.use("/api", h_send); check(r, "/api/"); }
        { test_router r; r.use("/api", h_send); check(r, "/api/v1"); }
        { test_router r; r.use("/api", h_next); r.use("/api", h_send); check(r, "/api"); }

        // multiple paths
        { test_router r; r.use("/x", h_next); r.use("/y", h_send); check(r, "/y"); }
        { test_router r; r.use("/x", h_next); r.use("/", h_send); check(r, "/"); }

        // path adjustments
        { test_router r; r.use("/api", h_path("/api", "/")); check(r, "/api"); }
        { test_router r; r.use("/api", h_path("/api", "/v1")); check(r, "/api/v1"); }
        { test_router r; r.use("/api/v1", h_path("/api/v1", "/")); check(r, "/api/v1"); }
    }

    void testRoute()
    {
        static auto const GET = http::method::get;
        static auto const POST = http::method::post;

        // basic routing
        { test_router r; r.add(GET, "/", h_send); check(r, GET, "/"); }
        { test_router r; r.add(GET, "/", h_next); check(r, POST, "/", route::next); }
        { test_router r; r.add(POST, "/", h_send); check(r, POST, "/"); }

        // verb matching - case sensitive
        { test_router r; r.add(GET, "/", h_send); check(r, "GET", "/"); }
        { test_router r; r.add(GET, "/", h_next); check(r, "get", "/", route::next); }

        // custom verb
        { test_router r; r.add("CUSTOM", "/", h_send); check(r, "CUSTOM", "/"); }
        { test_router r; r.add("CUSTOM", "/", h_next); check(r, "custom", "/", route::next); }

        // path matching
        { test_router r; r.add(GET, "/x", h_next); r.add(GET, "/y", h_send); check(r, GET, "/y"); }

        // route().add()
        { test_router r; r.route("/").add(GET, h_send); check(r, GET, "/"); }
        { test_router r; r.route("/").add(GET, h_next).add(POST, h_send); check(r, POST, "/"); }

        // all() matches any method
        { test_router r; r.all("/", h_send); check(r, GET, "/"); }
        { test_router r; r.all("/", h_send); check(r, POST, "/"); }
        { test_router r; r.all("/", h_send); check(r, "CUSTOM", "/"); }

        // route::next_route skips to next route
        { test_router r; r.route("/").add(GET, h_next_route).add(GET, h_next); r.route("/").add(GET, h_send); check(r, GET, "/"); }

        // multiple handlers on same route
        { test_router r; r.route("/").add(GET, h_next).add(GET, h_send); check(r, GET, "/"); }
    }

    void testError()
    {
        system::error_code const er = http::error::bad_connection;
        system::error_code const er2 = http::error::bad_expect;

        // error from handler
        { test_router r; r.use(h_fail(er)); check(r, "/", er); }

        // error handler catches
        { test_router r; r.use(h_fail(er), eh_send(er)); check(r, "/"); }

        // error handler next
        { test_router r; r.use(h_fail(er), eh_next(er), eh_send(er)); check(r, "/"); }

        // error handler returns new error
        { test_router r; r.use(h_fail(er), eh_return(er2), eh_send(er2)); check(r, "/"); }

        // error skips plain handlers
        { test_router r; r.use(h_fail(er), h_next, eh_send(er)); check(r, "/"); }

        // error from route skips to middleware
        { test_router r; r.route("/").add(http::method::get, h_fail(er)); r.use(eh_send(er)); check(r, http::method::get, "/"); }

        // error handler on different path not called
        { test_router r; r.use("/x", h_fail(er)); r.use("/x", eh_send(er)); check(r, "/x"); }
    }

    void testException()
    {
#ifndef BOOST_NO_EXCEPTIONS
        // unhandled exception
        { test_router r; r.use(h_throw); check(r, "/", error::unhandled_exception); }

        // exception handler catches
        { test_router r; r.use(h_throw); r.except(exh_send()); check(r, "/"); }

        // exception handler next
        { test_router r; r.use(h_throw); r.except(exh_next(), exh_send()); check(r, "/"); }
#endif
    }

    void testNested()
    {
        static auto const GET = http::method::get;

        // nested router middleware
        {
            test_router r;
            r.use("/api", []{
                test_router r2;
                r2.use("/v1", h_send);
                return r2;
            }());
            check(r, "/api/v1");
        }

        // nested router path adjustments
        {
            test_router r;
            r.use("/api", []{
                test_router r2;
                r2.use("/v1", h_path("/api/v1", "/"));
                return r2;
            }());
            check(r, "/api/v1");
        }

        // nested router with routes
        {
            test_router r;
            r.use("/api", []{
                test_router r2;
                r2.add(GET, "/user", h_send);
                return r2;
            }());
            check(r, GET, "/api/user");
        }

        // deeply nested
        {
            test_router r;
            r.use("/a", []{
                test_router r2;
                r2.use("/b", []{
                    test_router r3;
                    r3.use("/c", h_send);
                    return r3;
                }());
                return r2;
            }());
            check(r, "/a/b/c");
        }

        // nested path restoration on sibling skip
        {
            test_router r;
            r.use("/api", []{
                test_router r2;
                r2.use("/v1", h_next);  // doesn't match /api/v2
                return r2;
            }());
            r.use("/api", h_send);  // should match /api/v2
            check(r, "/api/v2");
        }

        // sibling routers - first doesn't match
        {
            test_router r;
            r.use("/x", h_next);
            r.use("/y", h_send);
            check(r, "/y");
        }

        // error propagates from nested
        {
            system::error_code const er = http::error::bad_connection;
            test_router r;
            r.use("/api", [er]{
                test_router r2;
                r2.use(h_fail(er));
                return r2;
            }());
            r.use(eh_send(er));
            check(r, "/api");
        }

        // nesting beyond max_path_depth throws
        {
            // Build a chain of routers exceeding max_path_depth
            auto make_deep = [](auto& self, std::size_t depth) -> test_router
            {
                test_router r;
                if(depth > 0)
                    r.use("/x", self(self, depth - 1));
                else
                    r.use(h_send);
                return r;
            };

            // max_path_depth levels should succeed
            {
                test_router r;
                r.use("/a", make_deep(make_deep,
                    detail::router_base::max_path_depth - 2));
                // Should not throw
                flat_router fr(std::move(r));
                (void)fr;
            }

            // max_path_depth + 1 should throw
            BOOST_TEST_THROWS(
                [&]{
                    test_router r;
                    r.use("/a", make_deep(make_deep,
                        detail::router_base::max_path_depth));
                }(),
                std::length_error);
        }
    }

    void testOptions()
    {
        static auto const GET = http::method::get;

        // case_sensitive (default false)
        { test_router r; r.use("/api", h_send); check(r, "/API"); }
        { test_router r; r.add(GET, "/api", h_send); check(r, GET, "/API"); }

        // case_sensitive true
        {
            test_router r(router_options().case_sensitive(true));
            r.use("/api", h_next);
            check(r, "/API", route::next);
        }

        // nested inherits options
        {
            test_router r(router_options().case_sensitive(true));
            r.use("/api", []{
                test_router r2;  // inherits case_sensitive
                r2.use("/v1", h_next);
                return r2;
            }());
            check(r, "/api/V1", route::next);
        }

        // nested can override options
        {
            test_router r(router_options().case_sensitive(true));
            r.use("/api", []{
                test_router r2(router_options().case_sensitive(false));
                r2.use("/v1", h_send);
                return r2;
            }());
            check(r, "/api/V1");
        }
    }

    void testDispatch()
    {
        // unknown method throws
        {
            test_router r;
            r.use(h_next);
            flat_router fr(std::move(r));
            params req;
            BOOST_TEST_THROWS(
                capy::run_sync()(fr.dispatch(
                    http::method::unknown, urls::url_view("/"), req)),
                std::invalid_argument);
        }

        // empty verb string throws
        {
            test_router r;
            r.use(h_next);
            flat_router fr(std::move(r));
            params req;
            BOOST_TEST_THROWS(
                capy::run_sync()(fr.dispatch(
                    "", urls::url_view("/"), req)),
                std::invalid_argument);
        }
    }

    void testPathDecoding()
    {
        static auto const GET = http::method::get;

        // percent-encoded characters
        { test_router r; r.add(GET, "/a", h_send); check(r, GET, "/%61"); }
        { test_router r; r.add(GET, "/%61", h_send); check(r, GET, "/a"); }

        // encoded slash doesn't match path separator
        { test_router r; r.add(GET, "/auth/login", h_next); check(r, GET, "/auth%2flogin", route::next); }
    }

    void run()
    {
        testUse();
        testRoute();
        testError();
        testException();
        testNested();
        testOptions();
        testDispatch();
        testPathDecoding();
    }
};

TEST_SUITE(
    router_test,
    "boost.http.server.router");

} // http
} // boost
