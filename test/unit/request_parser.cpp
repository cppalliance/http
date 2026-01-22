//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/request_parser.hpp>
#include <boost/http/rfc/combine_field_values.hpp>

#include <boost/http/core/polystore.hpp>

#include "test_suite.hpp"

#include <algorithm>
#include <string>

namespace boost {
namespace http {

struct request_parser_test
{
    bool
    feed(
        parser& pr,
        core::string_view s)
    {
        while(! s.empty())
        {
            auto b = *pr.prepare().begin();
            auto n = b.size();
            if( n > s.size())
                n = s.size();
            std::memcpy(b.data(),
                s.data(), n);
            pr.commit(n);
            s.remove_prefix(n);
            system::error_code ec;
            pr.parse(ec);
            if(ec == error::end_of_message
                || ! ec)
                break;
            if(! BOOST_TEST(
                ec == condition::need_more_input))
            {
                test_suite::log <<
                    ec.message() << "\n";
                return false;
            }
        }
        return true;
    }

    bool
    valid(
        request_parser& pr,
        core::string_view s,
        std::size_t nmax)
    {
        pr.reset();
        pr.start();
        while(! s.empty())
        {
            auto b = *pr.prepare().begin();
            auto n = b.size();
            if( n > s.size())
                n = s.size();
            if( n > nmax)
                n = nmax;
            std::memcpy(b.data(),
                s.data(), n);
            pr.commit(n);
            s.remove_prefix(n);
            system::error_code ec;
            pr.parse(ec);
            if(ec == condition::need_more_input)
                continue;
            auto const es = ec.message();
            return ! ec.failed();
        }
        return false;
    }

    void
    good(
        http::polystore& ctx,
        core::string_view s,
        core::string_view expected = {})
    {
        for(std::size_t nmax = 1;
            nmax < s.size(); ++nmax)
        {
            request_parser pr(ctx);
            if( BOOST_TEST(valid(pr, s, nmax)) )
            {
                BOOST_TEST( pr.got_header() );
                if( expected.data() )
                {
                    auto const& req = pr.get();
                    BOOST_TEST_EQ(req.buffer(), expected);
                }
            }
        }
    }

    void
    bad(http::polystore& ctx, core::string_view s)
    {
        for(std::size_t nmax = 1;
            nmax < s.size(); ++nmax)
        {
            request_parser pr(ctx);
            BOOST_TEST(! valid(pr, s, nmax));
        }
    }

    void
    check(
        method m,
        core::string_view t,
        version v,
        core::string_view const s)
    {
        auto const f =
            [&](request_parser const& pr)
        {
            auto const& req = pr.get();
            BOOST_TEST(req.method() == m);
            BOOST_TEST(req.method_text() ==
                to_string(m));
            BOOST_TEST(req.target() == t);
            BOOST_TEST(req.version() == v);
        };

        http::polystore ctx;
        request_parser::config cfg;
        install_parser_service(ctx, cfg);

        // single buffer
        {
            request_parser pr(ctx);
            pr.reset();
            pr.start();
            auto const b = *pr.prepare().begin();
            auto const n = (std::min)(
                b.size(), s.size());
            BOOST_TEST(n == s.size());
            std::memcpy(
                b.data(), s.data(), n);
            pr.commit(n);
            system::error_code ec;
            pr.parse(ec);
            BOOST_TEST(! ec);
            //BOOST_TEST(pr.is_done());
            if(! ec)
                f(pr);
        }

        // two buffers
        for(std::size_t i = 1;
            i < s.size(); ++i)
        {
            request_parser pr(ctx);
            pr.reset();
            pr.start();
            // first buffer
            auto b = *pr.prepare().begin();
            auto n = (std::min)(
                b.size(), i);
            BOOST_TEST(n == i);
            std::memcpy(
                b.data(), s.data(), n);
            pr.commit(n);
            system::error_code ec;
            pr.parse(ec);
            if(! BOOST_TEST(
                ec == condition::need_more_input))
                continue;
            // second buffer
            b = *pr.prepare().begin();
            n = (std::min)(
                b.size(), s.size());
            BOOST_TEST(n == s.size());
            std::memcpy(
                b.data(), s.data() + i, n - i);
            pr.commit(n);
            pr.parse(ec);
            if(ec.failed())
                continue;
            //BOOST_TEST(pr.is_done());
            f(pr);
        }
    }

    //--------------------------------------------

    void
    testSpecial()
    {
        // request_parser()
        {
            BOOST_TEST_NO_THROW(request_parser());
        }

        // operator=(request_parser&&)
        {
            request_parser pr;
            BOOST_TEST_NO_THROW(pr = request_parser());
        }
        {
            request_parser pr;

            http::polystore ctx;
            request_parser::config cfg;
            install_parser_service(ctx, cfg);

            BOOST_TEST_NO_THROW(pr = request_parser(ctx));
        }

        // request_parser(http::polystore&)
        {
            http::polystore ctx;
            request_parser::config cfg;
            install_parser_service(ctx, cfg);
            request_parser pr(ctx);
        }

        // request_parser(request_parser&&)
        {
            http::polystore ctx;
            install_parser_service(ctx, {});
            request_parser pr1(ctx);
            request_parser pr2(std::move(pr1));
        }
        {
            BOOST_TEST_NO_THROW(
                request_parser(request_parser()));
        }
    }

    //--------------------------------------------

    void
    testParse()
    {
        check(method::get, "/",
            version::http_1_1,
            "GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "Host: localhost\r\n"
            "\r\n");
    }

    void
    testParseField()
    {
        auto f = [](core::string_view f)
        {
            return std::string(
                "GET / HTTP/1.1\r\n") +
                std::string(
                    f.data(), f.size()) +
                "\r\n\r\n";
        };

        http::polystore ctx;
        request_parser::config cfg;
        install_parser_service(ctx, cfg);

        bad(ctx, f(":"));
        bad(ctx, f(" :"));
        bad(ctx, f(" x:"));
        bad(ctx, f("x :"));
        bad(ctx, f("x@"));
        bad(ctx, f("x@:"));
        bad(ctx, f("\ra"));
        bad(ctx, f("x:   \r"));
        bad(ctx, f("x:   \ra"));
        bad(ctx, f("x:   \r\nabcd"));
        bad(ctx, f("x:   a\x01""bcd"));

        good(ctx, f(""));
        good(ctx, f("x:"));
        good(ctx, f("x: "));
        good(ctx, f("x:\t "));
        good(ctx, f("x:y"));
        good(ctx, f("x: y"));
        good(ctx, f("x:y "));
        good(ctx, f("x: y "));
        good(ctx, f("x:yy"));
        good(ctx, f("x: yy"));
        good(ctx, f("x:yy "));
        good(ctx, f("x: y y "));
        good(ctx, f("x:"));
        good(ctx, f("x:   \r\n"),
                  f("x:   "));
        good(ctx, f("x:\r\n"),
                  f("x:"));

        // obs-fold handling
        good(ctx, f("x: \r\n "),
                  f("x:    "));
        good(ctx, f("x: \r\n x"),
                  f("x:    x"));
        good(ctx, f("x: \r\n \t\r\n \r\n\t "),
                  f("x:    \t     \t "));
        good(ctx, f("x: \r\n \t\r\n x"),
                  f("x:    \t   x"));
        good(ctx, f("x: y \r\n "),
                  f("x: y    "));
        good(ctx, f("x: \t\r\n abc def \r\n\t "),
                  f("x: \t   abc def   \t "));
        good(ctx, f("x: abc\r\n def"),
                  f("x: abc   def"));

        // errata eid4189
        good(ctx, f("x: , , ,"));
        good(ctx, f("x: abrowser/0.001 (C O M M E N T)"));
        good(ctx, f("x: gzip , chunked"));
    }

    void
    testGet()
    {
        http::polystore ctx;
        request_parser::config cfg;
        install_parser_service(ctx, cfg);
        request_parser pr(ctx);
        core::string_view s =
            "GET / HTTP/1.1\r\n"
            "Accept: *\r\n"
            "User-Agent: x\r\n"
            "Connection: close\r\n"
            "a: 1\r\n"
            "b: 2\r\n"
            "a: 3\r\n"
            "c: 4\r\n"
            "\r\n";

        pr.reset();
        pr.start();
        feed(pr, s);

        auto const& req = pr.get();
        BOOST_TEST(
            req.method() == method::get);
        BOOST_TEST(
            req.method_text() == "GET");
        BOOST_TEST(req.target() == "/");
        BOOST_TEST(req.version() ==
            version::http_1_1);

        BOOST_TEST(req.buffer() == s);
        BOOST_TEST(req.size() == 7);
        BOOST_TEST(
            req.exists(field::connection));
        BOOST_TEST(! req.exists(field::age));
        BOOST_TEST(req.exists("Connection"));
        BOOST_TEST(req.exists("CONNECTION"));
        BOOST_TEST(! req.exists("connector"));
        BOOST_TEST(req.count(field::accept) == 1);
        BOOST_TEST(
            req.count(field::age) == 0);
        BOOST_TEST(
            req.count("connection") == 1);
        BOOST_TEST(req.count("a") == 2);
        BOOST_TEST(req.find(
            field::connection)->id ==
                field::connection);
        BOOST_TEST(
            req.find("a")->value == "1");
        grammar::recycled_ptr<std::string> temp;
        BOOST_TEST(combine_field_values(req.find_all(
            field::user_agent), temp) == "x");
        BOOST_TEST(combine_field_values(req.find_all(
            "b"), temp) == "2");
        BOOST_TEST(combine_field_values(req.find_all(
            "a"), temp) == "1,3");
    }

    void
    run()
    {
        testSpecial();
        testParse();
        testParseField();
        testGet();
    }
};

TEST_SUITE(
    request_parser_test,
    "boost.http.request_parser");

} // http
} // boost
