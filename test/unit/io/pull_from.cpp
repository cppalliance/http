//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/io/pull_from.hpp>
#include <boost/capy/task.hpp>

#include <boost/http/test/buffer_sink.hpp>
#include <boost/http/test/read_source.hpp>
#include <boost/capy/test/read_stream.hpp>

#include "test_helpers.hpp"

#include <string_view>

namespace boost {
namespace http {
namespace {
using namespace capy;
using namespace capy::test;

class pull_from_test
{
public:
    //-------------------------------------------------------------------
    // ReadSource → BufferSink tests
    //-------------------------------------------------------------------

    void
    testReadSourceToBufferSinkEmpty()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f);
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            // ReadSource returns capy::error::eof when empty, but pull_from handles it
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 0u);
            BOOST_TEST(bs.data().empty());
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadSourceToBufferSinkSingle()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f);
            rs.provide("hello world");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadSourceToBufferSinkMultiple()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f);
            rs.provide("hello");
            rs.provide(" ");
            rs.provide("world");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadSourceToBufferSinkPartialRead()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f, 5); // max 5 bytes per read
            rs.provide("hello world");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadSourceToBufferSinkLarge()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f);
            std::string large_data(10000, 'x');
            rs.provide(large_data);
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 10000u);
            BOOST_TEST_EQ(bs.size(), 10000u);
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadSourceToBufferSinkSmallSinkBuffer()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f);
            rs.provide("hello world");
            test::buffer_sink bs(f, 5); // small buffer

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadSourceToBufferSinkSourceError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f);
            rs.provide("test data");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
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
    testReadSourceToBufferSinkSinkError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            test::read_source rs(f);
            rs.provide("test data");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
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
    // ReadStream → BufferSink tests
    //-------------------------------------------------------------------

    void
    testReadStreamToBufferSinkEmpty()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f);
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 0u);
            BOOST_TEST(bs.data().empty());
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadStreamToBufferSinkSingle()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f);
            rs.provide("hello world");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadStreamToBufferSinkMultiple()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f);
            rs.provide("hello");
            rs.provide(" ");
            rs.provide("world");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadStreamToBufferSinkPartialRead()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f, 3); // max 3 bytes per read
            rs.provide("hello world");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadStreamToBufferSinkLarge()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f);
            std::string large_data(10000, 'y');
            rs.provide(large_data);
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 10000u);
            BOOST_TEST_EQ(bs.size(), 10000u);
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadStreamToBufferSinkSmallSinkBuffer()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f);
            rs.provide("hello world");
            test::buffer_sink bs(f, 4); // small buffer

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST_EQ(bs.data(), "hello world");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadStreamToBufferSinkChunked()
    {
        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f, 7); // max 7 bytes per read
            rs.provide("hello world test data");
            test::buffer_sink bs(f, 5); // max 5 bytes per prepare

            auto [ec, n] = co_await pull_from(rs, bs);
            if(ec)
                co_return;

            BOOST_TEST_EQ(n, 21u);
            BOOST_TEST_EQ(bs.data(), "hello world test data");
            BOOST_TEST(bs.eof_called());
        });
        BOOST_TEST(r.success);
    }

    void
    testReadStreamToBufferSinkStreamError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f);
            rs.provide("test data");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
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
    testReadStreamToBufferSinkSinkError()
    {
        int error_count = 0;
        int success_count = 0;

        capy::test::fuse f;
        auto r = f.armed([&](capy::test::fuse&) -> capy::task<> {
            capy::test::read_stream rs(f);
            rs.provide("test data");
            test::buffer_sink bs(f);

            auto [ec, n] = co_await pull_from(rs, bs);
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
        // ReadSource → BufferSink tests
        testReadSourceToBufferSinkEmpty();
        testReadSourceToBufferSinkSingle();
        testReadSourceToBufferSinkMultiple();
        testReadSourceToBufferSinkPartialRead();
        testReadSourceToBufferSinkLarge();
        testReadSourceToBufferSinkSmallSinkBuffer();
        testReadSourceToBufferSinkSourceError();
        testReadSourceToBufferSinkSinkError();

        // ReadStream → BufferSink tests
        testReadStreamToBufferSinkEmpty();
        testReadStreamToBufferSinkSingle();
        testReadStreamToBufferSinkMultiple();
        testReadStreamToBufferSinkPartialRead();
        testReadStreamToBufferSinkLarge();
        testReadStreamToBufferSinkSmallSinkBuffer();
        testReadStreamToBufferSinkChunked();
        testReadStreamToBufferSinkStreamError();
        testReadStreamToBufferSinkSinkError();
    }
};

TEST_SUITE(pull_from_test, "boost.capy.io.pull_from");

} // namespace
} // capy
} // boost
