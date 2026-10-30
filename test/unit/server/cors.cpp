//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/cors.hpp>

#include "test_worker.hpp"

#include <chrono>

namespace boost {
namespace http {

struct cors_test
    : test_worker
{
    // Shorthand to build a router with cors + handler
    static test_router
    make_cors_router(cors_options opts)
    {
        test_router r;
        r.use(cors(std::move(opts)));
        r.use(h_text("ok"));
        return r;
    }

    // Preflight request with optional extra headers
    static std::string
    preflight(
        std::string_view path = "/",
        std::string_view extra = {})
    {
        return make_request(
            OPTIONS, path, extra);
    }

    //--------------------------------------------
    // Preflight defaults
    //--------------------------------------------

    void
    testPreflightDefaults()
    {
        auto r = make_cors_router({});
        auto res = exchange(r, preflight());

        // Default status is 204 No Content
        BOOST_TEST(res.status() ==
            status::no_content);

        // Wildcard origin
        BOOST_TEST_EQ(res.res.value_or(
            field::access_control_allow_origin, ""),
            "*");

        // Default methods
        BOOST_TEST_EQ(res.res.value_or(
            field::access_control_allow_methods, ""),
            "GET,HEAD,PUT,PATCH,POST,DELETE");

        // No credentials by default
        BOOST_TEST_NOT(res.res.exists(
            field::access_control_allow_credentials));

        // No max-age by default
        BOOST_TEST_NOT(res.res.exists(
            field::access_control_max_age));

        // No exposed headers by default
        BOOST_TEST_NOT(res.res.exists(
            field::access_control_expose_headers));
    }

    //--------------------------------------------
    // Preflight with specific origin
    //--------------------------------------------

    void
    testPreflightOrigin()
    {
        // Explicit wildcard
        {
            cors_options o;
            o.origin = "*";
            auto r = make_cors_router(o);
            auto res = exchange(r, preflight());
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin, ""),
                "*");
            // No Vary for wildcard
            BOOST_TEST_NOT(res.res.exists(
                field::vary));
        }

        // Specific origin adds Vary
        {
            cors_options o;
            o.origin = "https://example.com";
            auto r = make_cors_router(o);
            auto res = exchange(r, preflight());
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin, ""),
                "https://example.com");
            BOOST_TEST(res.res.exists(field::vary));
        }
    }

    //--------------------------------------------
    // Preflight methods
    //--------------------------------------------

    void
    testPreflightMethods()
    {
        cors_options o;
        o.methods = "GET,POST";
        auto r = make_cors_router(o);
        auto res = exchange(r, preflight());
        BOOST_TEST_EQ(res.res.value_or(
            field::access_control_allow_methods, ""),
            "GET,POST");
    }

    //--------------------------------------------
    // Preflight credentials
    //--------------------------------------------

    void
    testPreflightCredentials()
    {
        cors_options o;
        o.credentials = true;
        auto r = make_cors_router(o);
        auto res = exchange(r, preflight());
        BOOST_TEST_EQ(res.res.value_or(
            field::access_control_allow_credentials,
            ""),
            "true");
    }

    //--------------------------------------------
    // Preflight allowed headers
    //--------------------------------------------

    void
    testPreflightAllowedHeaders()
    {
        // Explicit allowed headers
        {
            cors_options o;
            o.allowedHeaders = "X-Custom, Authorization";
            auto r = make_cors_router(o);
            auto res = exchange(r, preflight());
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_headers,
                ""),
                "X-Custom, Authorization");
        }

        // Echo from request header
        {
            cors_options o;
            auto r = make_cors_router(o);
            auto res = exchange(r, preflight("/",
                "Access-Control-Request-Headers: "
                "X-Foo, X-Bar\r\n"));
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_headers,
                ""),
                "X-Foo, X-Bar");
        }

        // No request header, no echoed header
        {
            cors_options o;
            auto r = make_cors_router(o);
            auto res = exchange(r, preflight());
            BOOST_TEST_NOT(res.res.exists(
                field::access_control_allow_headers));
        }
    }

    //--------------------------------------------
    // Preflight max-age
    //--------------------------------------------

    void
    testPreflightMaxAge()
    {
        cors_options o;
        o.max_age = std::chrono::seconds(3600);
        auto r = make_cors_router(o);
        auto res = exchange(r, preflight());
        BOOST_TEST_EQ(res.res.value_or(
            field::access_control_max_age, ""),
            "3600");
    }

    //--------------------------------------------
    // Preflight exposed headers
    //--------------------------------------------

    void
    testPreflightExposedHeaders()
    {
        cors_options o;
        o.exposedHeaders = "X-Request-Id";
        auto r = make_cors_router(o);
        auto res = exchange(r, preflight());
        BOOST_TEST_EQ(res.res.value_or(
            field::access_control_expose_headers, ""),
            "X-Request-Id");
    }

    //--------------------------------------------
    // Preflight custom status
    //--------------------------------------------

    void
    testPreflightStatus()
    {
        cors_options o;
        o.result = status::ok;
        auto r = make_cors_router(o);
        auto res = exchange(r, preflight());
        BOOST_TEST(res.status() == status::ok);
    }

    //--------------------------------------------
    // preFlightContinue passes to next handler
    //--------------------------------------------

    void
    testPreflightContinue()
    {
        cors_options o;
        o.preFlightContinue = true;
        auto r = make_cors_router(o);
        auto res = exchange(r, preflight());

        // Next handler sends "ok" with 200
        BOOST_TEST(res.status() == status::ok);
        BOOST_TEST(res.body == "ok");

        // CORS headers still present
        BOOST_TEST_EQ(res.res.value_or(
            field::access_control_allow_origin, ""),
            "*");
        BOOST_TEST(res.res.exists(
            field::access_control_allow_methods));
    }

    //--------------------------------------------
    // Normal (non-OPTIONS) request gets headers
    //--------------------------------------------

    void
    testNormalRequest()
    {
        // Default options
        {
            auto r = make_cors_router({});
            auto res = exchange(r, GET, "/");
            BOOST_TEST(res.status() == status::ok);
            BOOST_TEST(res.body == "ok");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin,
                ""),
                "*");
            // No methods header on normal response
            BOOST_TEST_NOT(res.res.exists(
                field::access_control_allow_methods));
            // No max-age on normal response
            BOOST_TEST_NOT(res.res.exists(
                field::access_control_max_age));
            // No allowed-headers on normal response
            BOOST_TEST_NOT(res.res.exists(
                field::access_control_allow_headers));
        }

        // Credentials on normal response
        {
            cors_options o;
            o.credentials = true;
            auto r = make_cors_router(o);
            auto res = exchange(r, GET, "/");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_credentials,
                ""),
                "true");
        }

        // Exposed headers on normal response
        {
            cors_options o;
            o.exposedHeaders = "X-RateLimit";
            auto r = make_cors_router(o);
            auto res = exchange(r, GET, "/");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_expose_headers,
                ""),
                "X-RateLimit");
        }

        // Specific origin on normal response
        {
            cors_options o;
            o.origin = "https://app.example.com";
            auto r = make_cors_router(o);
            auto res = exchange(r, GET, "/");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin,
                ""),
                "https://app.example.com");
            BOOST_TEST(res.res.exists(field::vary));
        }
    }

    //--------------------------------------------
    // Full options combined
    //--------------------------------------------

    void
    testFullOptions()
    {
        cors_options o;
        o.origin = "https://example.com";
        o.methods = "GET,POST,DELETE";
        o.allowedHeaders = "Authorization, Content-Type";
        o.exposedHeaders = "X-Request-Id, X-RateLimit";
        o.max_age = std::chrono::seconds(86400);
        o.credentials = true;

        auto r = make_cors_router(o);

        // Preflight
        {
            auto res = exchange(r, preflight());
            BOOST_TEST(res.status() ==
                status::no_content);
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin, ""),
                "https://example.com");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_methods, ""),
                "GET,POST,DELETE");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_headers, ""),
                "Authorization, Content-Type");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_expose_headers, ""),
                "X-Request-Id, X-RateLimit");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_max_age, ""),
                "86400");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_credentials,
                ""),
                "true");
        }

        // Normal GET
        {
            auto res = exchange(r, GET, "/");
            BOOST_TEST(res.status() == status::ok);
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin, ""),
                "https://example.com");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_credentials,
                ""),
                "true");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_expose_headers, ""),
                "X-Request-Id, X-RateLimit");
            // Preflight-only headers absent
            BOOST_TEST_NOT(res.res.exists(
                field::access_control_allow_methods));
            BOOST_TEST_NOT(res.res.exists(
                field::access_control_max_age));
            BOOST_TEST_NOT(res.res.exists(
                field::access_control_allow_headers));
        }
    }

    //--------------------------------------------
    // CORS with route-level handler
    //--------------------------------------------

    void
    testWithRoutes()
    {
        test_router r;
        r.use(cors(cors_options{}));
        r.add(GET, "/api", h_text("data"));
        r.add(POST, "/api", h_text("created"));

        // GET /api
        {
            auto res = exchange(r, GET, "/api");
            BOOST_TEST(res.status() == status::ok);
            BOOST_TEST(res.body == "data");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin, ""),
                "*");
        }

        // POST /api
        {
            auto res = exchange(r, POST, "/api");
            BOOST_TEST(res.status() == status::ok);
            BOOST_TEST(res.body == "created");
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin, ""),
                "*");
        }

        // OPTIONS /api (preflight)
        {
            auto res = exchange(r, preflight("/api"));
            BOOST_TEST(res.status() ==
                status::no_content);
            BOOST_TEST(res.res.exists(
                field::access_control_allow_origin));
        }

        // Unmatched path still gets CORS on 404
        {
            auto res = exchange(r, GET, "/missing");
            BOOST_TEST(res.status() ==
                status::not_found);
            BOOST_TEST_EQ(res.res.value_or(
                field::access_control_allow_origin, ""),
                "*");
        }
    }

    //--------------------------------------------
    // Fuse: preflight error injection
    //--------------------------------------------

    void
    testFusePreflight()
    {
        auto r = make_cors_router({});
        auto fr = fused(r, OPTIONS, "/",
            [](auto&, auto& w)
        {
            capy::test::run_blocking()(
                w.do_http_session());
        });
        BOOST_TEST(fr.success);
    }

    //--------------------------------------------
    // Fuse: normal request error injection
    //--------------------------------------------

    void
    testFuseNormal()
    {
        auto r = make_cors_router({});
        auto fr = fused(r, GET, "/",
            [](auto&, auto& w)
        {
            capy::test::run_blocking()(
                w.do_http_session());
        });
        BOOST_TEST(fr.success);
    }

    //--------------------------------------------
    // Fuse: full options error injection
    //--------------------------------------------

    void
    testFuseFullOptions()
    {
        cors_options o;
        o.origin = "https://example.com";
        o.methods = "GET,POST";
        o.allowedHeaders = "Authorization";
        o.exposedHeaders = "X-Request-Id";
        o.max_age = std::chrono::seconds(3600);
        o.credentials = true;
        auto r = make_cors_router(o);

        // Preflight under fuse
        {
            auto fr = fused(r, OPTIONS, "/",
                [](auto&, auto& w)
            {
                capy::test::run_blocking()(
                    w.do_http_session());
            });
            BOOST_TEST(fr.success);
        }

        // Normal request under fuse
        {
            auto fr = fused(r, GET, "/",
                [](auto&, auto& w)
            {
                capy::test::run_blocking()(
                    w.do_http_session());
            });
            BOOST_TEST(fr.success);
        }
    }

    //--------------------------------------------
    // Fuse: preFlightContinue error injection
    //--------------------------------------------

    void
    testFusePreflightContinue()
    {
        cors_options o;
        o.preFlightContinue = true;
        auto r = make_cors_router(o);
        auto fr = fused(r, OPTIONS, "/",
            [](auto&, auto& w)
        {
            capy::test::run_blocking()(
                w.do_http_session());
        });
        BOOST_TEST(fr.success);
    }

    void
    run()
    {
        testPreflightDefaults();
        testPreflightOrigin();
        testPreflightMethods();
        testPreflightCredentials();
        testPreflightAllowedHeaders();
        testPreflightMaxAge();
        testPreflightExposedHeaders();
        testPreflightStatus();
        testPreflightContinue();
        testNormalRequest();
        testFullOptions();
        testWithRoutes();
        testFusePreflight();
        testFuseNormal();
        testFuseFullOptions();
        testFusePreflightContinue();
    }
};

TEST_SUITE(
    cors_test,
    "boost.http.server.cors");

} // http
} // boost
