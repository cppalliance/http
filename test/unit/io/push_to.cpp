//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/io/push_to.hpp>
#include <boost/capy/task.hpp>

#include <boost/http/test/buffer_source.hpp>
#include <boost/http/test/write_sink.hpp>
#include <boost/capy/test/write_stream.hpp>

#include "test_helpers.hpp"

#include <string_view>

namespace boost {
namespace http {
namespace {
using namespace capy;
using namespace capy::test;

class push_to_test
{
public:
    //-------------------------------------------------------------------
    // BufferSource → WriteSink tests
    //-------------------------------------------------------------------

    void
    testBufferSourceToWriteSinkEmpty()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            test::write_sink ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 0u);
            BOOST_TEST(ws.data().empty());
            BOOST_TEST(ws.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteSinkSingle()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("hello world");
            test::write_sink ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(ws.data(), "hello world");
            BOOST_TEST(ws.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteSinkMultiple()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("hello");
            bs.provide(" ");
            bs.provide("world");
            test::write_sink ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(ws.data(), "hello world");
            BOOST_TEST(ws.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteSinkChunked()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f, 5); // max 5 bytes per pull
            bs.provide("hello world");
            test::write_sink ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(ws.data(), "hello world");
            BOOST_TEST(ws.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteSinkLarge()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            std::string large_data(10000, 'x');
            bs.provide(large_data);
            test::write_sink ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 10000u);
            BOOST_TEST_EQ(ws.size(), 10000u);
            BOOST_TEST(ws.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteSinkSourceError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("test data");
            test::write_sink ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
            {
                ++error_count;
                co_return;
            }
            ++success_count;
        });

        BOOST_TEST(r.success);
        BOOST_TEST(error_count > 0);
        BOOST_TEST(success_count > 0);
    }

    void
    testBufferSourceToWriteSinkSinkError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("test data");
            test::write_sink ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
            {
                ++error_count;
                co_return;
            }
            ++success_count;
        });

        BOOST_TEST(r.success);
        BOOST_TEST(error_count > 0);
        BOOST_TEST(success_count > 0);
    }

    //-------------------------------------------------------------------
    // BufferSource → WriteStream tests
    //-------------------------------------------------------------------

    void
    testBufferSourceToWriteStreamEmpty()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            capy::test::write_stream ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 0u);
            BOOST_TEST(ws.data().empty());
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteStreamSingle()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("hello world");
            capy::test::write_stream ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(ws.data(), "hello world");
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteStreamMultiple()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("hello");
            bs.provide(" ");
            bs.provide("world");
            capy::test::write_stream ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(ws.data(), "hello world");
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteStreamPartialWrite()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("hello world");
            capy::test::write_stream ws(f, 3); // max 3 bytes per write

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(ws.data(), "hello world");
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteStreamLarge()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            std::string large_data(10000, 'y');
            bs.provide(large_data);
            capy::test::write_stream ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 10000u);
            BOOST_TEST_EQ(ws.size(), 10000u);
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteStreamChunked()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f, 7); // max 7 bytes per pull
            bs.provide("hello world test data");
            capy::test::write_stream ws(f, 5); // max 5 bytes per write

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 21u);
            BOOST_TEST_EQ(ws.data(), "hello world test data");
        });
        BOOST_TEST(r.success);
    }

    void
    testBufferSourceToWriteStreamSourceError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("test data");
            capy::test::write_stream ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
            {
                ++error_count;
                co_return;
            }
            ++success_count;
        });

        BOOST_TEST(r.success);
        BOOST_TEST(error_count > 0);
        BOOST_TEST(success_count > 0);
    }

    void
    testBufferSourceToWriteStreamStreamError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::buffer_source bs(f);
            bs.provide("test data");
            capy::test::write_stream ws(f);

            auto [ec, n] = co_await push_to(bs, ws);
            if(ec)
            {
                ++error_count;
                co_return;
            }
            ++success_count;
        });

        BOOST_TEST(r.success);
        BOOST_TEST(error_count > 0);
        BOOST_TEST(success_count > 0);
    }

    void
    run()
    {
        // BufferSource → WriteSink tests
        testBufferSourceToWriteSinkEmpty();
        testBufferSourceToWriteSinkSingle();
        testBufferSourceToWriteSinkMultiple();
        testBufferSourceToWriteSinkChunked();
        testBufferSourceToWriteSinkLarge();
        testBufferSourceToWriteSinkSourceError();
        testBufferSourceToWriteSinkSinkError();

        // BufferSource → WriteStream tests
        testBufferSourceToWriteStreamEmpty();
        testBufferSourceToWriteStreamSingle();
        testBufferSourceToWriteStreamMultiple();
        testBufferSourceToWriteStreamPartialWrite();
        testBufferSourceToWriteStreamLarge();
        testBufferSourceToWriteStreamChunked();
        testBufferSourceToWriteStreamSourceError();
        testBufferSourceToWriteStreamStreamError();
    }
};

TEST_SUITE(push_to_test, "boost.capy.io.push_to");

} // namespace
} // capy
} // boost
