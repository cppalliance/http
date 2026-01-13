//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Christian Mazakas
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/serializer.hpp>
#include <boost/http/response.hpp>

#include <boost/capy/buffers/copy.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/buffers/slice.hpp>
#include <boost/capy/buffers/string_buffer.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/capy/polystore.hpp>

#include "test_helpers.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace boost {
namespace http {

struct serializer_test
{
    struct test_source : source
    {
        test_source(core::string_view s)
            : s_(s)
        {
        }

        results
        on_read(
            capy::mutable_buffer b) override
        {
            BOOST_TEST(! is_done_);
            results rv;
            rv.bytes =
                capy::copy(
                    b,
                    capy::make_buffer(
                        s_.data(),
                        s_.size()));
            s_ = s_.substr(rv.bytes);
            rv.finished = s_.empty();
            is_done_ = rv.finished;
            return rv;
        }

    private:
        core::string_view s_;
        bool is_done_ = false;
    };

    struct faulty_source : source
    {
        faulty_source(
            system::error_code ec)
            : ec_{ ec }
        {
        }

        bool
        is_done() const
        {
            return is_done_;
        }

        results
        on_read(capy::mutable_buffer) override
        {
            BOOST_TEST(!is_done_);
            is_done_ = true;
            results rv;
            rv.ec = ec_;
            return rv;
        }

    private:
        system::error_code ec_;
        bool is_done_ = false;
    };

    template<
        class ConstBuffers>
    static
    std::size_t
    append(
        std::string& dest,
        ConstBuffers const& src)
    {
        auto n0 = dest.size();
        auto n = capy::buffer_size(src);
        dest.resize(n0 + n);
        capy::copy(
            capy::mutable_buffer(
                &dest[n0], n), src);
        return n;
    }

    static
    std::string
    read_some(serializer& sr)
    {
        capy::slice_of<
            serializer::const_buffers_type> cbs
                = sr.prepare().value();
        BOOST_TEST(!sr.is_done());
        // We limit buffer consumption to necessitate
        // multiple calls to serializer::prepare() and
        // serializer::consume(), allowing tests to cover
        // state management within these functions
        std::string s;
        capy::keep_prefix(cbs, 256);
        for( auto buf : cbs)
        {
            s.append(
                reinterpret_cast<char const*>(buf.data()),
                buf.size());
            sr.consume(buf.size());
        }
        return s;
    }

    static
    std::string
    read(serializer& sr)
    {
        std::string s;
        while(! sr.is_done())
            s += read_some(sr);
        return s;
    }

    static
    void
    check_chunked_body(
        core::string_view chunked_body,
        core::string_view expected_contents)
    {
        for(;;)
        {
            auto n = chunked_body.find_first_of("\r\n");
            BOOST_TEST_NE(n, core::string_view::npos);
            std::string tmp = chunked_body.substr(0, n);
            chunked_body.remove_prefix(n + 2);
            auto chunk_size = std::stoul(tmp, nullptr, 16);

            if( chunk_size == 0 ) // last chunk
            {
                BOOST_TEST(chunked_body == "\r\n");
                chunked_body.remove_prefix(2);
                break;
            }

            BOOST_TEST_GE(expected_contents.size(), chunk_size);
            BOOST_TEST(chunked_body.starts_with(
                expected_contents.substr(0, chunk_size)));
            chunked_body.remove_prefix(chunk_size);
            expected_contents.remove_prefix(chunk_size);

            BOOST_TEST(chunked_body.starts_with("\r\n"));
            chunked_body.remove_prefix(2);
        }
        BOOST_TEST(chunked_body.empty());
        BOOST_TEST(expected_contents.empty());
    }

    //--------------------------------------------

    void
    testSyntax()
    {
        capy::polystore ctx;
        install_serializer_service(ctx, {});
        serializer sr(ctx);
        response res;

        sr.start(res);
        sr.reset();

        sr.start(res, capy::const_buffer{});
        sr.reset();

        sr.start(res, capy::mutable_buffer{});
        sr.reset();

        sr.start<test_source>(res, "12345");
        sr.reset();

        sr.start(res, capy::const_buffer{});
        sr.reset();

        sr.start(res, capy::mutable_buffer{});
        sr.reset();

        sr.start<test_source>(res, "12345");
        sr.reset();

        sr.start_stream(res);
        sr.reset();
    }

    void
    testSpecial()
    {
        capy::polystore ctx;
        install_serializer_service(ctx, {});
        response res;
        res.set_chunked(true);

        std::string expected = res.buffer();
        // empty body final chunk
        expected.append("0\r\n\r\n");

        // serializer()
        {
            BOOST_TEST_NO_THROW(serializer());
        }

        // operator=(serializer&&)
        {
            serializer sr;
            BOOST_TEST_NO_THROW(sr = serializer());
        }
        {
            serializer sr;
            BOOST_TEST_NO_THROW(sr = serializer(ctx));
        }

        // serializer(serializer&&)
        {
            std::string message;
            capy::string_buffer buf(&message);
            serializer sr1(ctx);
            sr1.start(res);

            // consume 5 bytes
            {
                auto cbs = sr1.prepare().value();
                auto n = capy::copy(buf.prepare(5), cbs);
                sr1.consume(n);
                buf.commit(n);
                BOOST_TEST_EQ(n, 5);
            }

            serializer sr2(std::move(sr1));

            // consume the reset from sr2
            {
                auto cbs = sr2.prepare().value();
                auto n = capy::copy(
                    buf.prepare(capy::buffer_size(cbs)),
                    cbs);
                sr2.consume(n);
                buf.commit(n);
            }

            BOOST_TEST(sr2.is_done());
            BOOST_TEST(message == expected);
        }
    }

    //--------------------------------------------

    void
    testEmptyBody()
    {
        auto const check =
        [](
            core::string_view headers,
            core::string_view expected)
        {
            response res(headers);
            capy::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);
            sr.start(res);
            std::string s = read(sr);
            BOOST_TEST(s == expected);
        };

        check(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 0\r\n"
            "\r\n");

        check(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "0\r\n\r\n");
    }

    //--------------------------------------------

    void
    check_buffers(
        core::string_view headers,
        core::string_view body,
        core::string_view expected_header,
        core::string_view expected_body)
    {
        response res(headers);
        std::array<
            capy::const_buffer, 23> buf;

        const auto buf_size =
            (body.size() / buf.size()) + 1;
        for(auto& cb : buf)
        {
            if(body.size() < buf_size)
            {
                cb = { body.data(), body.size() };
                body.remove_prefix(body.size());
                break;
            }
            cb = { body.data(), buf_size };
            body.remove_prefix(buf_size);
        }

        capy::polystore ctx;
        install_serializer_service(ctx, {});
        serializer sr(ctx);
        sr.start(res, buf);
        std::string s = read(sr);
        core::string_view sv(s);

        BOOST_TEST(
            sv.substr(0, expected_header.size())
                == expected_header);

        BOOST_TEST(
            sv.substr(expected_header.size())
                == expected_body);
    };

    template<class Source, class F>
    void
    check_src(
        core::string_view headers,
        Source&& src,
        F const& f)
    {
        response res(headers);
        // we limit the buffer size of the serializer, requiring
        // it to make multiple calls to source::read
        capy::polystore ctx;
        install_serializer_service(ctx, {});
        serializer sr(ctx);
        sr.start<Source>(res, std::forward<
            Source>(src));
        std::string s = read(sr);
        f(s);
    }

    template <class F>
    void
    check_stream(
        core::string_view headers,
        core::string_view body,
        F f)
    {
        response res(headers);
        capy::polystore ctx;
        install_serializer_service(ctx, {});
        serializer sr(ctx);
        auto stream = sr.start_stream(res);
        BOOST_TEST_GT(
            stream.capacity(),
            serializer::config{}.payload_buffer);

        std::vector<char> s; // stores complete output

        auto prepare = [&]()
        {
            BOOST_TEST(stream.capacity() != 0);
            auto mbs = stream.prepare();
            auto bs = capy::buffer_size(mbs);
            BOOST_TEST_EQ(bs, stream.capacity());

            if( bs > body.size() )
                bs = body.size();

            capy::copy(
                mbs, capy::const_buffer(body.data(), bs));

            stream.commit(bs);
            if( bs < body.size() )
                BOOST_TEST(stream.capacity() == 0);
            else
                BOOST_TEST(stream.capacity() != 0);

            body.remove_prefix(bs);
        };

        auto consume = [&](capy::const_buffer buf)
        {
            // we have the prepared buffer sequence
            // representing the serializer's output but we
            // wish to emulate a user consuming it using a
            // smaller, fixed-size buffer
            std::array<char, 16> storage{};
            capy::mutable_buffer out_buf(
                storage.data(), storage.size());

            while( buf.size() > 0 )
            {
                auto n = capy::copy(out_buf, buf);
                capy::remove_prefix(buf, n);

                s.insert(
                    s.end(),
                    storage.begin(),
                    storage.begin() + n);

                sr.consume(n);
            }
        };

        bool closed = false;
        while(! sr.is_done() )
        {
            if(! body.empty() )
                prepare();

            if( body.empty() && !closed )
            {
                stream.close();
                closed = true;
            }

            auto mcbs = sr.prepare();
            BOOST_TEST(!mcbs.has_error());

            auto cbs = mcbs.value();
            auto end = cbs.end();
            auto size = capy::buffer_size(cbs);
            BOOST_TEST_GT(size, 0);

            for(auto pos = cbs.begin(); pos != end; ++pos)
                consume(*pos);
        }

        f(core::string_view(s.data(), s.size()));
    }

    void
    testOutput()
    {
        // buffers (0 size)
        check_buffers(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            "",
            //--------------------------
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            "");

        // buffers
        check_buffers(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 2048\r\n"
            "\r\n",
            std::string(2048, '*'),
            //--------------------------
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 2048\r\n"
            "\r\n",
            std::string(2048, '*'));

        // buffers chunked
        check_buffers(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            std::string(2048, '*'),
            //--------------------------
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            std::string("800\r\n") +
            std::string(2048, '*') +
            std::string("\r\n0\r\n\r\n"));

        // buffers chunked empty
        check_buffers(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            "",
            //--------------------------
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            "0\r\n\r\n");

        // source
        check_src(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 2048\r\n"
            "\r\n",
            test_source{std::string(2048, '*')},
            [](core::string_view s){
                core::string_view header =
                    "HTTP/1.1 200 OK\r\n"
                    "Server: test\r\n"
                    "Content-Length: 2048\r\n"
                    "\r\n";

                BOOST_TEST(
                    s.substr(0, header.size()) == header);
                BOOST_TEST(s ==
                    "HTTP/1.1 200 OK\r\n"
                    "Server: test\r\n"
                    "Content-Length: 2048\r\n"
                    "\r\n" +
                    std::string(2048, '*'));
            });

        // empty source
        check_src(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            test_source{""},
            [](core::string_view s){
                BOOST_TEST(s ==
                    "HTTP/1.1 200 OK\r\n"
                    "Server: test\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n");
            });

        // faulty source
        {
            response res(
                "HTTP/1.1 200 OK\r\n"
                "\r\n");
            capy::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            auto& source = sr.start<faulty_source>(
                res, system::error_code(4224, system::system_category()));

            auto rs = sr.prepare();
            BOOST_TEST(rs.has_error());
            BOOST_TEST_EQ(rs.error().value(), 4224);
            BOOST_TEST(source.is_done());
            BOOST_TEST(sr.is_done() == false);
            BOOST_TEST_THROWS(
                sr.prepare(),
                std::logic_error);
            BOOST_TEST_THROWS(
                sr.start(res),
                std::logic_error);
            // reset faulty state and serialize a new message
            sr.reset();
            BOOST_TEST(sr.is_done() == true);
            sr.start(res);
            BOOST_TEST(sr.is_done() == false);
        }

        // source chunked
        check_src(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            test_source{std::string(2048, '*')},
            [](core::string_view s){
                core::string_view expected_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Server: test\r\n"
                    "Transfer-Encoding: chunked\r\n"
                    "\r\n";
                BOOST_TEST(s.starts_with(expected_header));
                s.remove_prefix(expected_header.size());
                check_chunked_body(s, std::string(2048, '*'));
            });

        // empty source chunked
        check_src(
            "HTTP/1.1 200 OK\r\n"
            "Server: test\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            test_source{""},
            [](core::string_view s){
                core::string_view expected_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Server: test\r\n"
                    "Transfer-Encoding: chunked\r\n"
                    "\r\n";
                BOOST_TEST(s.starts_with(expected_header));
                s.remove_prefix(expected_header.size());
                check_chunked_body(s, "");
            });

        // empty stream
        {
            check_stream(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "\r\n",
                std::string(0, '*'),
                [](core::string_view s)
                {
                    core::string_view expected_header =
                        "HTTP/1.1 200 OK\r\n"
                        "Server: test\r\n"
                        "\r\n";
                    BOOST_TEST(s.starts_with(expected_header));
                    s.remove_prefix(expected_header.size());
                    BOOST_TEST(s.empty());
                });
        }

        // empty stream, chunked
        {
            check_stream(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n",
                std::string(0, '*'),
                [](core::string_view s)
                {
                    core::string_view expected_header =
                        "HTTP/1.1 200 OK\r\n"
                        "Server: test\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "\r\n";
                    BOOST_TEST(s.starts_with(expected_header));
                    s.remove_prefix(expected_header.size());
                    check_chunked_body(s, "");
                });
        }

        // stream
        {
            check_stream(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 13370\r\n"
                "\r\n",
                std::string(13370, '*'),
                [](core::string_view s){
                    core::string_view expected_header =
                        "HTTP/1.1 200 OK\r\n"
                        "Server: test\r\n"
                        "Content-Length: 13370\r\n"
                        "\r\n";

                    BOOST_TEST(
                        s.starts_with(expected_header));

                    s.remove_prefix(expected_header.size());
                    BOOST_TEST(
                        s == std::string(13370, '*'));
                });
        }

        // stream, chunked
        {
            check_stream(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n",
                std::string(13370, '*'),
                [](core::string_view s)
                {
                    core::string_view expected_header =
                        "HTTP/1.1 200 OK\r\n"
                        "Server: test\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "\r\n";
                    BOOST_TEST(s.starts_with(
                        expected_header));
                    s.remove_prefix(expected_header.size());
                    check_chunked_body(
                        s, std::string(13370, '*'));
                });
        }
    }

    void
    testExpect100Continue()
    {
        // request
        {
            capy::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);
            request req(
                "GET / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Content-Length: 5\r\n"
                "\r\n");
            sr.start<test_source>(req, "12345");
            std::string s;
            system::result<
                serializer::const_buffers_type> rv;
            for(;;)
            {
                rv = sr.prepare();
                if(! rv)
                    break;
                auto n = append(s, *rv);
                sr.consume(n);
            }
            BOOST_TEST(rv.error() ==
                error::expect_100_continue);
            BOOST_TEST(s ==
                "GET / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Content-Length: 5\r\n"
                "\r\n");
            while(! sr.is_done())
            {
                rv = sr.prepare();
                BOOST_TEST(! rv.has_error());
                auto n = append(s, *rv);
                sr.consume(n);
            }
            BOOST_TEST_THROWS(
                sr.prepare(), std::logic_error);
            BOOST_TEST(s ==
                "GET / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Content-Length: 5\r\n"
                "\r\n"
                "12345");
        }

        // empty body
        {
            capy::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);
            request req(
                "GET / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Content-Length: 5\r\n"
                "\r\n");
            sr.start(req);
            std::string s;
            system::result<
                serializer::const_buffers_type> rv;
            for(;;)
            {
                rv = sr.prepare();
                if(! rv)
                    break;
                auto n = append(s, *rv);
                sr.consume(n);
            }
            BOOST_TEST(rv.error() ==
                error::expect_100_continue);
            BOOST_TEST(s ==
                "GET / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Content-Length: 5\r\n"
                "\r\n");
            BOOST_TEST(! sr.is_done());
            rv = sr.prepare();
            BOOST_TEST(! rv.has_error());
            BOOST_TEST_EQ(
                capy::buffer_size(*rv), 0);
            BOOST_TEST(! sr.is_done());
            sr.consume(0);
            BOOST_TEST(sr.is_done());
        }

        // response
        {
            core::string_view sv =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 5\r\n"
                "Expect: 100-continue\r\n"
                "\r\n";

            capy::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);
            response res(sv);
            sr.start<test_source>(res, "12345");
            auto s = read(sr);
            BOOST_TEST(s ==
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 5\r\n"
                "Expect: 100-continue\r\n"
                "\r\n"
                "12345");
        }
    }

    void
    testStreamErrors()
    {
        // Default constructor
        {
            serializer::stream stream;
            BOOST_TEST(!stream.is_open());
        }

        // Empty commits 
        {
            core::string_view sv =
                "HTTP/1.1 200 OK\r\n"
                "\r\n";
            response res(sv);
            capy::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);
            auto stream = sr.start_stream(res);

            // consume whole header
            {
                auto cbs = sr.prepare();
                BOOST_TEST_EQ(
                    capy::buffer_size(cbs.value()),
                    sv.size());
                sr.consume(sv.size());
            }

            // error::need_data
            {
                auto cbs = sr.prepare();
                BOOST_TEST_EQ(
                    cbs.error(),
                    error::need_data);
            }

            // commit 0
            {
                stream.prepare();
                stream.commit(0);
                auto cbs = sr.prepare();
                BOOST_TEST_EQ(
                    cbs.error(),
                    error::need_data);
            }

            // close empty
            {
                BOOST_TEST(!sr.is_done());
                stream.close();
                auto cbs = sr.prepare();
                BOOST_TEST_EQ(
                    capy::buffer_size(cbs.value()),
                    0);
                BOOST_TEST(!sr.is_done());
                sr.consume(0);
                BOOST_TEST(sr.is_done());
            }
        }

        {
            core::string_view sv =
                "HTTP/1.1 200 OK\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n";
            response res(sv);
            capy::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);
            auto stream = sr.start_stream(res);
            BOOST_TEST(stream.is_open());

            auto mbs = stream.prepare();
            BOOST_TEST_GT(
                capy::buffer_size(mbs), 0);
            BOOST_TEST(stream.capacity() != 0);

            // commit with `n > stream.capacity()`
            BOOST_TEST_THROWS(
                stream.commit(capy::buffer_size(mbs) + 1),
                std::invalid_argument);

            // commiting 0 bytes must be possible
            stream.commit(0);

            auto mcbs = sr.prepare();
            auto cbs = mcbs.value();
            BOOST_TEST_EQ(
                capy::buffer_size(cbs),
                sv.size());
            sr.consume(sv.size());

            mcbs = sr.prepare();
            BOOST_TEST(mcbs.has_error());
            BOOST_TEST(
                mcbs.error() == error::need_data);

            stream.close();
            BOOST_TEST(!stream.is_open());
            BOOST_TEST_THROWS(
                stream.prepare(),
                std::logic_error);
            BOOST_TEST_THROWS(
                stream.capacity(),
                std::logic_error);
            BOOST_TEST_THROWS(
                stream.commit(0),
                std::logic_error);

            stream.close(); // fine no-op

            mcbs = sr.prepare();
            std::string body;
            append(body, *mcbs);
            BOOST_TEST(body == "0\r\n\r\n");
            sr.consume(5);

            BOOST_TEST(sr.is_done());
            BOOST_TEST_THROWS(
                sr.consume(1),
                std::logic_error);
        }
    }

    void
    testOverConsume()
    {
        capy::polystore ctx;
        install_serializer_service(ctx, {});
        serializer sr(ctx);
        request req;
        sr.start(req);
        auto cbs = sr.prepare().value();
        sr.consume(capy::buffer_size(cbs) + 1);
        BOOST_TEST(sr.is_done());
    }

    void
    run()
    {
        testSyntax();
        testSpecial();
        testEmptyBody();
        testOutput();
        testExpect100Continue();
        testStreamErrors();
        testOverConsume();
    }
};

TEST_SUITE(
    serializer_test,
    "boost.http.serializer");

} // http
} // boost
