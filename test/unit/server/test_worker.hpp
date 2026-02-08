//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_TEST_WORKER_HPP
#define BOOST_HTTP_TEST_WORKER_HPP

#include <boost/http/server/http_worker.hpp>
#include <boost/capy/test/stream.hpp>
#include <boost/capy/test/run_blocking.hpp>

#include "test_helpers.hpp"

#include <string>

namespace boost {
namespace http {

/** Shared test utilities for http_worker round-trips.

    All members are static. Use as a namespace:
    @code
    using tw = test_worker;
    tw::check(tw::GET, "/hello", tw::h_text("world"),
        status::ok, "world");
    @endcode
*/
struct test_worker
{
    using test_router = router<route_params>;

    static constexpr auto GET     = method::get;
    static constexpr auto POST    = method::post;
    static constexpr auto PUT     = method::put;
    static constexpr auto HEAD    = method::head;
    static constexpr auto DELETE_ = method::delete_;
    static constexpr auto PATCH   = method::patch;

    //--------------------------------------------
    // Cached configs
    //--------------------------------------------

    static shared_parser_config const&
    pcfg()
    {
        static auto const cfg =
            make_parser_config(parser_config{true});
        return cfg;
    }

    static shared_serializer_config const&
    scfg()
    {
        static auto const cfg =
            make_serializer_config(
                serializer_config{});
        return cfg;
    }

    //--------------------------------------------
    // Handler helpers
    //--------------------------------------------

    /** Handler that sends 200 with text body.

        Captured string is safe: the router stores
        the handler in stable heap storage.
    */
    static auto
    h_text(std::string_view body)
    {
        return [body = std::string(body)](
            route_params& rp) -> route_task
        {
            auto [ec] = co_await rp.send(body);
            if(ec)
                co_return route_error(ec);
            co_return route_done;
        };
    }

    /** Handler that sends the given status with
        no body.
    */
    static auto
    h_stat(http::status s)
    {
        return [s](route_params& rp) -> route_task
        {
            auto [ec] = co_await rp.status(s).send();
            if(ec)
                co_return route_error(ec);
            co_return route_done;
        };
    }

    /** Handler that sends 200 with text body and
        a custom Content-Type.
    */
    static auto
    h_typed(
        std::string_view content_type,
        std::string_view body)
    {
        return [
            ct = std::string(content_type),
            body = std::string(body)](
                route_params& rp) -> route_task
        {
            rp.res.set(field::content_type, ct);
            auto [ec] = co_await rp.send(body);
            if(ec)
                co_return route_error(ec);
            co_return route_done;
        };
    }

    //--------------------------------------------
    // Parsed response
    //--------------------------------------------

    struct result
    {
        http::response res;
        std::string body;

        http::status
        status() const noexcept
        {
            return res.status();
        }
    };

    //--------------------------------------------
    // Build minimal HTTP/1.1 request
    //--------------------------------------------

    static std::string
    make_request(
        http::method m,
        std::string_view path,
        std::string_view extra_headers = {})
    {
        std::string s;
        auto ms = to_string(m);
        s.append(ms.data(), ms.size());
        s += ' ';
        s += path;
        s +=
            " HTTP/1.1\r\n"
            "Host: localhost\r\n";
        if(! extra_headers.empty())
            s += extra_headers;
        s +=
            "Connection: close\r\n"
            "\r\n";
        return s;
    }

    //--------------------------------------------
    // Split raw wire data into headers + body
    //--------------------------------------------

    static result
    parse_response(std::string_view raw)
    {
        result r;
        auto pos = raw.find("\r\n\r\n");
        if(! BOOST_TEST(
            pos != std::string_view::npos))
            return r;
        try
        {
            r.res = http::response(
                raw.substr(0, pos + 4));
        }
        catch(std::exception const& e)
        {
            BOOST_TEST_FAIL();
            return r;
        }
        r.body.assign(raw.substr(pos + 4));
        return r;
    }

    //--------------------------------------------
    // Run one request/response exchange
    //--------------------------------------------

    /** Run a raw request through a worker and
        return the parsed response.
    */
    static result
    exchange(
        test_router const& r,
        std::string_view request)
    {
        auto [client, server] =
            capy::test::make_stream_pair();

        http_worker w(
            server,
            test_router(r),
            pcfg(),
            scfg());

        client.provide(request);
        client.close();

        capy::test::run_blocking()(
            w.do_http_session());

        return parse_response(client.data());
    }

    /** Build the request from method + path, then
        run it.
    */
    static result
    exchange(
        test_router const& r,
        http::method m,
        std::string_view path)
    {
        return exchange(r, make_request(m, path));
    }

    //--------------------------------------------
    // Fuse-driven exchange
    //--------------------------------------------

    /** Run a request/response exchange under a fuse.

        The caller provides a lambda that receives
        the client stream and the http_worker. The
        fuse re-runs the lambda at successive error
        injection points.

        @par Example
        @code
        test_router r;
        r.add(GET, "/hello", h_text("world"));
        auto fr = fused(r, GET, "/hello",
            [](auto& client, auto& w)
        {
            capy::test::run_blocking()(
                w.do_http_session());
        });
        BOOST_TEST(fr.success);
        @endcode
    */
    template<class F>
    static capy::test::fuse::result
    fused(
        test_router const& r,
        http::method m,
        std::string_view path,
        F&& fn)
    {
        auto req = make_request(m, path);
        capy::test::fuse f;
        return f.armed([&](capy::test::fuse& f)
        {
            auto [client, server] =
                capy::test::make_stream_pair(f);

            http_worker w(
                server,
                test_router(r),
                pcfg(),
                scfg());

            client.provide(req);
            client.close();

            fn(client, w);
        });
    }

    //--------------------------------------------
    // Check: router + method + path
    //--------------------------------------------

    static void
    check(
        test_router const& r,
        http::method m,
        std::string_view path,
        http::status expected)
    {
        auto res = exchange(r, m, path);
        BOOST_TEST(res.status() == expected);
    }

    static void
    check(
        test_router const& r,
        http::method m,
        std::string_view path,
        http::status expected,
        std::string_view expected_body)
    {
        auto res = exchange(r, m, path);
        BOOST_TEST(res.status() == expected);
        BOOST_TEST(res.body == expected_body);
    }

    //--------------------------------------------
    // Check: single route (one-liner)
    //--------------------------------------------

    template<class H>
    static void
    check(
        http::method m,
        std::string_view path,
        H&& handler,
        http::status expected)
    {
        test_router r;
        r.add(m, path,
            std::forward<H>(handler));
        check(r, m, path, expected);
    }

    template<class H>
    static void
    check(
        http::method m,
        std::string_view path,
        H&& handler,
        http::status expected,
        std::string_view expected_body)
    {
        test_router r;
        r.add(m, path,
            std::forward<H>(handler));
        check(r, m, path,
            expected, expected_body);
    }
};

} // http
} // boost

#endif
