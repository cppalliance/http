//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/serializer.hpp>

#include <boost/http/response.hpp>

#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/buffer_copy.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/error.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/ex/execution_context.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/concept/write_sink.hpp>
#include <boost/capy/concept/write_stream.hpp>

#include "test_helpers.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <stop_token>
#include <string>
#include <string_view>
#include <utility>

namespace boost {
namespace http {

namespace {

//----------------------------------------------------------
// Mock WriteStream for testing
//----------------------------------------------------------

struct mock_write_stream
{
    std::string data;
    std::size_t chunk_size = 65536;
    system::error_code forced_error;
    std::size_t error_after = std::size_t(-1);
    std::size_t written = 0;

    template<capy::ConstBufferSequence CB>
    auto write_some(CB const& buffers)
    {
        struct awaitable
        {
            mock_write_stream* self_;
            capy::const_buffer buf_;

            bool await_ready() const noexcept { return true; }

            void await_suspend(
                capy::coro,
                capy::executor_ref,
                std::stop_token) const noexcept
            {
            }

            std::pair<system::error_code, std::size_t>
            await_resume() noexcept
            {
                if(self_->forced_error)
                    return {self_->forced_error, 0};

                if(self_->written >= self_->error_after)
                    return {capy::error::test_failure, 0};

                std::size_t can_write = self_->error_after - self_->written;
                std::size_t to_write = (std::min)({
                    buf_.size(),
                    can_write,
                    self_->chunk_size});

                self_->data.append(
                    static_cast<char const*>(buf_.data()),
                    to_write);
                self_->written += to_write;
                return {{}, to_write};
            }
        };

        capy::const_buffer buf = *capy::begin(buffers);
        return awaitable{this, buf};
    }
};

static_assert(capy::WriteStream<mock_write_stream>);

//----------------------------------------------------------
// Test executor
//----------------------------------------------------------

struct test_executor
{
    int& dispatch_count_;

    explicit test_executor(int& count) noexcept
        : dispatch_count_(count)
    {
    }

    bool operator==(test_executor const& other) const noexcept
    {
        return &dispatch_count_ == &other.dispatch_count_;
    }

    struct test_context : capy::execution_context {};

    capy::execution_context& context() const noexcept
    {
        static test_context ctx;
        return ctx;
    }

    void on_work_started() const noexcept {}
    void on_work_finished() const noexcept {}

    capy::coro dispatch(capy::coro h) const noexcept
    {
        ++dispatch_count_;
        return h;
    }

    void post(capy::coro) const noexcept {}
};

static_assert(capy::Executor<test_executor>);

} // namespace

//----------------------------------------------------------
// serializer_sink_test
//----------------------------------------------------------

struct serializer_sink_test
{
    // Verify serializer::sink satisfies WriteSink concept
    static_assert(capy::WriteSink<
        serializer::sink<mock_write_stream>>);

    std::shared_ptr<serializer_config_impl const> sr_cfg_ =
        make_serializer_config(serializer_config{});

    //------------------------------------------------------
    // Basic functionality tests
    //------------------------------------------------------

    void
    testWriteBasic()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_payload_size(13);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            std::string_view body = "Hello, World!";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(body));

            BOOST_TEST(!ec.failed());
            BOOST_TEST_EQ(n, 13u);

            auto [ec2] = co_await sink.write_eof();
            BOOST_TEST(!ec2.failed());
            BOOST_TEST(sr.is_done());

            // Check underlying stream received the data
            BOOST_TEST(underlying.data.find("Hello, World!") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteWithEof()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_payload_size(5);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            // Write with eof=true in one call
            std::string_view body = "hello";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(body), true);

            BOOST_TEST(!ec.failed());
            BOOST_TEST_EQ(n, 5u);
            BOOST_TEST(sr.is_done());

            BOOST_TEST(underlying.data.find("hello") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteEmpty()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_payload_size(0);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            // Empty buffer
            auto [ec, n] = co_await sink.write(
                capy::const_buffer(nullptr, 0));

            BOOST_TEST(!ec.failed());
            BOOST_TEST_EQ(n, 0u);

            auto [ec2] = co_await sink.write_eof();
            BOOST_TEST(!ec2.failed());
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteMultiple()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_chunked(true);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            // First write
            std::string_view part1 = "Hello, ";
            auto [ec1, n1] = co_await sink.write(
                capy::const_buffer(part1.data(), part1.size()));
            BOOST_TEST(!ec1.failed());
            BOOST_TEST_EQ(n1, 7u);

            // Second write
            std::string_view part2 = "World!";
            auto [ec2, n2] = co_await sink.write(
                capy::const_buffer(part2.data(), part2.size()));
            BOOST_TEST(!ec2.failed());
            BOOST_TEST_EQ(n2, 6u);

            auto [ec3] = co_await sink.write_eof();
            BOOST_TEST(!ec3.failed());
            BOOST_TEST(sr.is_done());

            // Verify both parts in output
            BOOST_TEST(underlying.data.find("Hello, ") !=
                std::string::npos);
            BOOST_TEST(underlying.data.find("World!") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // write_eof tests
    //------------------------------------------------------

    void
    testWriteEofBasic()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_chunked(true);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            std::string_view body = "test";
            auto [ec1, n] = co_await sink.write(
                capy::make_buffer(body));
            BOOST_TEST(!ec1.failed());

            auto [ec2] = co_await sink.write_eof();
            BOOST_TEST(!ec2.failed());
            BOOST_TEST(sr.is_done());

            // Chunked output ends with "0\r\n\r\n"
            BOOST_TEST(underlying.data.find("0\r\n\r\n") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteEofEmpty()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_chunked(true);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            // Close without writing any body
            auto [ec] = co_await sink.write_eof();
            BOOST_TEST(!ec.failed());
            BOOST_TEST(sr.is_done());

            // Should have final chunk
            BOOST_TEST(underlying.data.find("0\r\n\r\n") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // HTTP message tests
    //------------------------------------------------------

    void
    testContentLengthBody()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_payload_size(5);
            res.set(field::content_type, "text/plain");
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            std::string_view body = "hello";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(body));
            BOOST_TEST(!ec.failed());
            BOOST_TEST_EQ(n, 5u);

            auto [ec2] = co_await sink.write_eof();
            BOOST_TEST(!ec2.failed());
            BOOST_TEST(sr.is_done());

            // Check Content-Length header
            BOOST_TEST(underlying.data.find("Content-Length: 5") !=
                std::string::npos);
            BOOST_TEST(underlying.data.find("hello") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testChunkedBody()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            serializer sr(sr_cfg_);

            response res;
            res.set_chunked(true);
            res.set(field::content_type, "text/plain");
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            std::string_view body = "chunked data";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(body));
            BOOST_TEST(!ec.failed());

            auto [ec2] = co_await sink.write_eof();
            BOOST_TEST(!ec2.failed());
            BOOST_TEST(sr.is_done());

            // Check Transfer-Encoding header
            BOOST_TEST(underlying.data.find("Transfer-Encoding: chunked") !=
                std::string::npos);
            // Check final chunk marker
            BOOST_TEST(underlying.data.find("0\r\n\r\n") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Error handling tests
    //------------------------------------------------------

    void
    testWriteError()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            underlying.forced_error = capy::error::test_failure;

            serializer sr(sr_cfg_);

            response res;
            res.set_chunked(true);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            std::string_view body = "data";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(body));

            // Error should be reported
            BOOST_TEST(ec.failed());
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteEofError()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = [this]() -> capy::task<void>
        {
            mock_write_stream underlying;
            underlying.error_after = 50;

            serializer sr(sr_cfg_);

            response res;
            res.set_chunked(true);
            auto sink = sr.get_sink(underlying);
            sr.start_stream(res);

            // Write some data
            std::string_view body = "Hello";
            (void)co_await sink.write(
                capy::make_buffer(body));

            // write_eof may report error
            auto [ec] = co_await sink.write_eof();
            (void)ec;
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------

    void
    run()
    {
        testWriteBasic();
        testWriteWithEof();
        testWriteEmpty();
        testWriteMultiple();
        testWriteEofBasic();
        testWriteEofEmpty();
        testContentLengthBody();
        testChunkedBody();
        testWriteError();
        testWriteEofError();
    }
};

TEST_SUITE(
    serializer_sink_test,
    "boost.http.serializer_sink");

} // namespace http
} // namespace boost
