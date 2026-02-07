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

#include <boost/capy/buffers/buffer_copy.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/buffers/slice.hpp>
#include <boost/capy/buffers/string_dynamic_buffer.hpp>
#include <boost/capy/concept/buffer_sink.hpp>
#include <boost/capy/io/any_buffer_sink.hpp>
#include <boost/capy/test/fuse.hpp>
#include <boost/capy/test/write_stream.hpp>
#include <boost/core/ignore_unused.hpp>

#include "test_helpers.hpp"

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace boost {
namespace http {

struct serializer_test
{
    std::shared_ptr<serializer_config_impl const> cfg_ =
        make_serializer_config(serializer_config{});

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
        capy::buffer_copy(
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
        serializer sr(cfg_);
        response res;

        sr.set_message(res);
        sr.start();
        sr.reset();

        sr.set_message(res);
        sr.start_writes();
        sr.stream_close();
        sr.reset();

        sr.set_message(res);
        sr.start_buffers();
        sr.stream_close();
        sr.reset();
    }

    void
    testSpecial()
    {
        response res;
        res.set_chunked(true);

        std::string expected = res.buffer();
        // empty body final chunk
        expected.append("0\r\n\r\n");

        // serializer() - default constructor (no state)
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
            BOOST_TEST_NO_THROW(sr = serializer(cfg_));
        }

        // serializer(serializer&&)
        {
            std::string message;
            capy::string_dynamic_buffer buf(&message);
            serializer sr1(cfg_);
            sr1.set_message(res);
            sr1.start();

            // consume 5 bytes
            {
                auto cbs = sr1.prepare().value();
                auto n = capy::buffer_copy(buf.prepare(5), cbs);
                sr1.consume(n);
                buf.commit(n);
                BOOST_TEST_EQ(n, 5);
            }

            serializer sr2(std::move(sr1));

            // consume the reset from sr2
            {
                auto cbs = sr2.prepare().value();
                auto n = capy::buffer_copy(
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
        [this](
            core::string_view headers,
            core::string_view expected)
        {
            response res(headers);
            serializer sr(cfg_);
            sr.set_message(res);
            sr.start();
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

    std::string
    sink_serialize(
        core::string_view headers,
        core::string_view body = {})
    {
        std::string result;
        response res(headers);
        capy::test::fuse f;
        auto r = f.armed(
            [this, &res, &result, body]
            (capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);
            if(body.empty())
            {
                auto [ec] = co_await sink.write_eof();
                if(ec)
                    co_return;
            }
            else
            {
                std::string_view sv(
                    body.data(), body.size());
                auto [ec, n] = co_await sink.write_eof(
                    capy::make_buffer(sv));
                if(ec)
                    co_return;
            }
            BOOST_TEST(sr.is_done());
            result = ws.data();
        });
        BOOST_TEST(r.success);
        return result;
    }

    template <class F>
    void
    check_stream(
        core::string_view headers,
        core::string_view body,
        F f)
    {
        response res(headers);
        serializer sr(cfg_);
        sr.set_message(res);
        sr.start_writes();
        BOOST_TEST(sr.stream_capacity() != 0);

        std::vector<char> s; // stores complete output

        auto prepare = [&]()
        {
            BOOST_TEST(sr.stream_capacity() != 0);
            auto mbs = sr.stream_prepare();
            auto bs = capy::buffer_size(mbs);
            BOOST_TEST_EQ(bs, sr.stream_capacity());

            if( bs > body.size() )
                bs = body.size();

            capy::buffer_copy(
                mbs, capy::const_buffer(body.data(), bs));

            sr.stream_commit(bs);
            if( bs < body.size() )
                BOOST_TEST(sr.stream_capacity() == 0);
            else
                BOOST_TEST(sr.stream_capacity() != 0);

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
                auto n = capy::buffer_copy(out_buf, buf);
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
                sr.stream_close();
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
        // WriteSink: Content-Length: 0
        {
            auto s = sink_serialize(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 0\r\n"
                "\r\n");
            BOOST_TEST(s ==
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 0\r\n"
                "\r\n");
        }

        // WriteSink: Content-Length: 2048
        {
            std::string body(2048, '*');
            auto s = sink_serialize(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 2048\r\n"
                "\r\n",
                body);
            BOOST_TEST(s ==
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 2048\r\n"
                "\r\n" + body);
        }

        // WriteSink: chunked 2048
        {
            std::string body(2048, '*');
            auto s = sink_serialize(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n",
                body);
            core::string_view sv(s);
            core::string_view hdr =
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n";
            BOOST_TEST(sv.starts_with(hdr));
            sv.remove_prefix(hdr.size());
            check_chunked_body(sv, body);
        }

        // WriteSink: chunked empty
        {
            auto s = sink_serialize(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n");
            core::string_view sv(s);
            core::string_view hdr =
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n";
            BOOST_TEST(sv.starts_with(hdr));
            sv.remove_prefix(hdr.size());
            BOOST_TEST(sv == "0\r\n\r\n");
        }

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
            serializer sr(cfg_);
            request req(
                "GET / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Content-Length: 5\r\n"
                "\r\n");
            sr.set_message(req);
            sr.start_writes();
            {
                auto mbp = sr.stream_prepare();
                capy::const_buffer body("12345", 5);
                std::size_t n = 0;
                for(auto const& mb : mbp)
                {
                    auto chunk =
                        (std::min)(mb.size(), body.size());
                    if(chunk == 0)
                        break;
                    std::memcpy(
                        mb.data(), body.data(), chunk);
                    body += chunk;
                    n += chunk;
                }
                sr.stream_commit(n);
            }
            sr.stream_close();
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
            serializer sr(cfg_);
            request req(
                "GET / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Content-Length: 5\r\n"
                "\r\n");
            sr.set_message(req);
            sr.start();
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

            serializer sr(cfg_);
            response res(sv);
            sr.set_message(res);
            sr.start_writes();
            {
                auto mbp = sr.stream_prepare();
                capy::const_buffer body("12345", 5);
                std::size_t n = 0;
                for(auto const& mb : mbp)
                {
                    auto chunk =
                        (std::min)(mb.size(), body.size());
                    if(chunk == 0)
                        break;
                    std::memcpy(
                        mb.data(), body.data(), chunk);
                    body += chunk;
                    n += chunk;
                }
                sr.stream_commit(n);
            }
            sr.stream_close();
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
        // Empty commits 
        {
            core::string_view sv =
                "HTTP/1.1 200 OK\r\n"
                "\r\n";
            response res(sv);
            serializer sr(cfg_);
            sr.set_message(res);
            sr.start_writes();

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
                sr.stream_prepare();
                sr.stream_commit(0);
                auto cbs = sr.prepare();
                BOOST_TEST_EQ(
                    cbs.error(),
                    error::need_data);
            }

            // close empty
            {
                BOOST_TEST(!sr.is_done());
                sr.stream_close();
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
            serializer sr(cfg_);
            sr.set_message(res);
            sr.start_writes();

            auto mbs = sr.stream_prepare();
            BOOST_TEST_GT(
                capy::buffer_size(mbs), 0);
            BOOST_TEST(sr.stream_capacity() != 0);

            // commit with `n > stream_capacity()`
            BOOST_TEST_THROWS(
                sr.stream_commit(capy::buffer_size(mbs) + 1),
                std::invalid_argument);

            // commiting 0 bytes must be possible
            sr.stream_commit(0);

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

            sr.stream_close();

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
        serializer sr(cfg_);
        request req;
        sr.set_message(req);
        sr.start();
        auto cbs = sr.prepare().value();
        sr.consume(capy::buffer_size(cbs) + 1);
        BOOST_TEST(sr.is_done());
    }

    //--------------------------------------------
    // Sink tests (BufferSink interface)
    //--------------------------------------------

    // Verify serializer::sink satisfies BufferSink concept
    static_assert(capy::BufferSink<
        serializer::sink<capy::test::write_stream>>);

    void
    testSinkCommitBasic()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_payload_size(13);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            capy::mutable_buffer arr[16];
            auto bufs = sink.prepare(arr);
            if(bufs.empty())
                co_return;

            std::string_view body = "Hello, World!";
            std::memcpy(bufs[0].data(), body.data(), body.size());

            auto [ec] = co_await sink.commit(body.size());
            if(ec)
                co_return;

            auto [ec2] = co_await sink.commit_eof(0);
            if(ec2)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("Hello, World!") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    void
    testSinkCommitWithEof()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_payload_size(5);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            capy::mutable_buffer arr[16];
            auto bufs = sink.prepare(arr);
            if(bufs.empty())
                co_return;

            std::string_view body = "hello";
            std::memcpy(bufs[0].data(), body.data(), body.size());

            auto [ec] = co_await sink.commit_eof(body.size());
            if(ec)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("hello") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    void
    testSinkCommitEmpty()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_payload_size(0);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            auto [ec] = co_await sink.commit(0);
            if(ec)
                co_return;

            auto [ec2] = co_await sink.commit_eof(0);
            if(ec2)
                co_return;
        });
        BOOST_TEST(r.success);
    }

    void
    testSinkCommitMultiple()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_chunked(true);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            // First commit
            capy::mutable_buffer arr[16];
            auto bufs = sink.prepare(arr);
            if(bufs.empty())
                co_return;

            std::string_view part1 = "Hello, ";
            std::memcpy(bufs[0].data(), part1.data(), part1.size());
            auto [ec1] = co_await sink.commit(part1.size());
            if(ec1)
                co_return;

            // Second commit
            bufs = sink.prepare(arr);
            if(bufs.empty())
                co_return;

            std::string_view part2 = "World!";
            std::memcpy(bufs[0].data(), part2.data(), part2.size());
            auto [ec2] = co_await sink.commit(part2.size());
            if(ec2)
                co_return;

            auto [ec3] = co_await sink.commit_eof(0);
            if(ec3)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("Hello, ") !=
                std::string::npos);
            BOOST_TEST(ws.data().find("World!") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    void
    testSinkCommitEofBasic()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_chunked(true);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            capy::mutable_buffer arr[16];
            auto bufs = sink.prepare(arr);
            if(bufs.empty())
                co_return;

            std::string_view body = "test";
            std::memcpy(bufs[0].data(), body.data(), body.size());
            auto [ec1] = co_await sink.commit(body.size());
            if(ec1)
                co_return;

            auto [ec2] = co_await sink.commit_eof(0);
            if(ec2)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("0\r\n\r\n") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    void
    testSinkCommitEofEmpty()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_chunked(true);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            auto [ec] = co_await sink.commit_eof(0);
            if(ec)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("0\r\n\r\n") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    void
    testSinkContentLength()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_payload_size(5);
            res.set(field::content_type, "text/plain");
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            capy::mutable_buffer arr[16];
            auto bufs = sink.prepare(arr);
            if(bufs.empty())
                co_return;

            std::string_view body = "hello";
            std::memcpy(bufs[0].data(), body.data(), body.size());
            auto [ec] = co_await sink.commit(body.size());
            if(ec)
                co_return;

            auto [ec2] = co_await sink.commit_eof(0);
            if(ec2)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("Content-Length: 5") !=
                std::string::npos);
            BOOST_TEST(ws.data().find("hello") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    void
    testSinkChunked()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_chunked(true);
            res.set(field::content_type, "text/plain");
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            capy::mutable_buffer arr[16];
            auto bufs = sink.prepare(arr);
            if(bufs.empty())
                co_return;

            std::string_view body = "chunked data";
            std::memcpy(bufs[0].data(), body.data(), body.size());
            auto [ec] = co_await sink.commit(body.size());
            if(ec)
                co_return;

            auto [ec2] = co_await sink.commit_eof(0);
            if(ec2)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("Transfer-Encoding: chunked") !=
                std::string::npos);
            BOOST_TEST(ws.data().find("0\r\n\r\n") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    //--------------------------------------------
    // WriteSink direct tests
    //--------------------------------------------

    void
    testWriteSinkMultiWrite()
    {
        // Multiple writes, Content-Length
        {
            response res(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 10\r\n"
                "\r\n");
            capy::test::fuse f;
            auto r = f.armed(
                [this, &res]
                (capy::test::fuse& f) -> capy::task<>
            {
                capy::test::write_stream ws(f);
                serializer sr(cfg_);
                sr.set_message(res);
                auto sink = sr.sink_for(ws);

                std::string_view s1("Hello");
                auto [ec1, n1] = co_await sink.write(
                    capy::make_buffer(s1));
                if(ec1)
                    co_return;
                BOOST_TEST_EQ(n1, 5u);

                std::string_view s2(", Bob");
                auto [ec2, n2] = co_await sink.write_eof(
                    capy::make_buffer(s2));
                if(ec2)
                    co_return;
                BOOST_TEST_EQ(n2, 5u);

                BOOST_TEST(sr.is_done());
                BOOST_TEST(ws.data().find("Hello, Bob") !=
                    std::string::npos);
            });
            BOOST_TEST(r.success);
        }

        // Multiple writes, chunked
        {
            response res(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n");
            capy::test::fuse f;
            auto r = f.armed(
                [this, &res]
                (capy::test::fuse& f) -> capy::task<>
            {
                capy::test::write_stream ws(f);
                serializer sr(cfg_);
                sr.set_message(res);
                auto sink = sr.sink_for(ws);

                std::string_view s1("Hello");
                auto [ec1, n1] = co_await sink.write(
                    capy::make_buffer(s1));
                if(ec1)
                    co_return;

                std::string_view s2("World");
                auto [ec2, n2] = co_await sink.write(
                    capy::make_buffer(s2));
                if(ec2)
                    co_return;

                auto [ec3] = co_await sink.write_eof();
                if(ec3)
                    co_return;

                BOOST_TEST(sr.is_done());
                BOOST_TEST(ws.data().find("0\r\n\r\n") !=
                    std::string::npos);
            });
            BOOST_TEST(r.success);
        }
    }

    void
    testWriteSinkLargeBody()
    {
        // Large body, Content-Length
        {
            std::string body(13370, '*');
            auto s = sink_serialize(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 13370\r\n"
                "\r\n",
                body);
            BOOST_TEST(s ==
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Content-Length: 13370\r\n"
                "\r\n" + body);
        }

        // Large body, chunked
        {
            std::string body(13370, '*');
            auto s = sink_serialize(
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n",
                body);
            core::string_view sv(s);
            core::string_view hdr =
                "HTTP/1.1 200 OK\r\n"
                "Server: test\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n";
            BOOST_TEST(sv.starts_with(hdr));
            sv.remove_prefix(hdr.size());
            check_chunked_body(sv, body);
        }
    }

    //--------------------------------------------
    // any_buffer_sink wrapper tests (WriteSink)
    //--------------------------------------------

    void
    testAnyBufferSinkWrite()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_payload_size(13);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            capy::any_buffer_sink abs(sink);

            std::string_view body = "Hello, World!";
            auto [ec, n] = co_await abs.write(
                capy::make_buffer(body));
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 13u);

            auto [ec2] = co_await abs.write_eof();
            if(ec2)
                co_return;

            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("Hello, World!") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
    }

    void
    testAnyBufferSinkWriteWithEof()
    {
        capy::test::fuse f;
        auto r = f.armed([this](capy::test::fuse& f) -> capy::task<>
        {
            capy::test::write_stream ws(f);
            serializer sr(cfg_);

            response res;
            res.set_payload_size(5);
            sr.set_message(res);
            auto sink = sr.sink_for(ws);

            capy::any_buffer_sink abs(sink);

            std::string_view body = "hello";
            auto [ec, n] = co_await abs.write_eof(
                capy::make_buffer(body));
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 5u);
            BOOST_TEST(sr.is_done());
            BOOST_TEST(ws.data().find("hello") !=
                std::string::npos);
        });
        BOOST_TEST(r.success);
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

        // Sink tests (BufferSink interface)
        testSinkCommitBasic();
        testSinkCommitWithEof();
        testSinkCommitEmpty();
        testSinkCommitMultiple();
        testSinkCommitEofBasic();
        testSinkCommitEofEmpty();
        testSinkContentLength();
        testSinkChunked();

        // WriteSink direct tests
        testWriteSinkMultiWrite();
        testWriteSinkLargeBody();

        // any_buffer_sink wrapper tests (WriteSink)
        testAnyBufferSinkWrite();
        testAnyBufferSinkWriteWithEof();
    }
};

TEST_SUITE(
    serializer_test,
    "boost.http.serializer");

} // http
} // boost
