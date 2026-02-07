//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/cors.hpp>
#include <boost/http/server/basic_router.hpp>
#include "src/rfc/detail/rules.hpp"

#include <boost/http/server/http_worker.hpp>
#include <boost/http/config.hpp>
#include <boost/capy/test/stream.hpp>
#include <boost/capy/test/fuse.hpp>
#include <boost/capy/test/run_blocking.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

#if 0
// DO NOT REMOVE THIS
class field_item
{
public:
    field_item(
        core::string_view s)
        : s_(s)
    {
        grammar::parse(s_,
            detail::field_name_rule).value();
    }

    field_item(
        field f) noexcept
        : s_(to_string(f))
    {
    }

    operator core::string_view() const noexcept
    {
        return s_;
    }

private:
    core::string_view s_;
};

template<class Element>
struct list
{
    struct item
    {
        core::string_view s;

        template<
            class T,
            class = typename std::enable_if<
                std::is_constructible<
                    Element, T>::value>::type>
        item(T&& t)
            : s(Element(std::forward<T>(t)))
        {
        }
    };

public:
    list(std::initializer_list<item> init)
    {
        if(init.size() == 0)
            return;
        auto it = init.begin();
        s_ = it->s;
        while(++it != init.end())
        {
            s_.push_back(',');
            s_.append(it->s.data(),
                it->s.size());
        }
    }

    core::string_view get() const noexcept
    {
        return s_;
    }

private:
    std::string s_;
};
#endif

struct cors_test
{
    void
    testPreflight()
    {
        http::cors_options opts;
        opts.preFlightContinue = true;

        http::router r;
        r.use(http::cors(opts));
        r.use([](http::route_params& rp) -> http::route_task {
            auto [ec] = co_await rp.send("ok");
            if(ec)
                co_return http::route_error(ec);
            co_return http::route_done;
        });
        auto parser_cfg = http::make_parser_config(
            http::parser_config{true});
        auto serializer_cfg = http::make_serializer_config(
            http::serializer_config{});

        capy::test::fuse f;
        auto [client, server] =
            capy::test::make_stream_pair(f);

        http::http_worker worker(
            server, std::move(r),
            parser_cfg, serializer_cfg);

        client.provide(
            "OPTIONS /test HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n"
            "\r\n");
        client.close();

        capy::test::run_blocking()(
            worker.do_http_session());

        auto response = client.data();
        BOOST_TEST(response.find("200") !=
            std::string_view::npos);
        BOOST_TEST(response.find(
            "Access-Control-Allow-Origin: *") !=
            std::string_view::npos);
        BOOST_TEST(response.find(
            "Access-Control-Allow-Methods: "
            "GET,HEAD,PUT,PATCH,POST,DELETE") !=
            std::string_view::npos);
    }

    void run()
    {
        testPreflight();

#if 0
        list<field_item> v({
            field::access_control_allow_origin,
            "example.com",
            "example.org"
            });
#endif
    }
};

TEST_SUITE(
    cors_test,
    "boost.http.server.cors");

} // http
} // boost
