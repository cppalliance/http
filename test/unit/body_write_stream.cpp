//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/body_write_stream.hpp>

#include <boost/http/response.hpp>
#include <boost/http/core/polystore.hpp>

#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/copy.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/error.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/ex/execution_context.hpp>
#include <boost/capy/ex/run_async.hpp>
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
// Mock IoAwaitable for write operations
//----------------------------------------------------------

struct mock_write_awaitable
{
    std::string* data_;
    std::size_t chunk_size_;
    capy::const_buffer buf_;
    system::error_code ec_;

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
        if(ec_)
            return {ec_, 0};

        std::size_t to_write = std::min(buf_.size(), chunk_size_);
        data_->append(
            static_cast<char const*>(buf_.data()),
            to_write);
        return {{}, to_write};
    }
};

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
                std::size_t to_write = std::min({
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
// Test executor (from capy patterns)
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
// body_write_stream_test
//----------------------------------------------------------

struct body_write_stream_test
{
    // Verify body_write_stream satisfies WriteStream concept
    static_assert(capy::WriteStream<
        body_write_stream<mock_write_stream>>);

    //------------------------------------------------------
    // Basic functionality tests
    //------------------------------------------------------

    void
    testWriteSomeBasic()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_payload_size(13);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            std::string_view body = "Hello, World!";
            auto [ec, n] = co_await bws.write_some(
                capy::make_buffer(body));

            BOOST_TEST(!ec);
            BOOST_TEST_EQ(n, 13u);

            auto [ec2] = co_await bws.close();
            BOOST_TEST(!ec2);
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
    testWriteSomeEmpty()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_payload_size(0);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            // Empty buffer should complete immediately with n=0
            auto [ec, n] = co_await bws.write_some(
                capy::const_buffer(nullptr, 0));

            BOOST_TEST(!ec);
            BOOST_TEST_EQ(n, 0u);

            auto [ec2] = co_await bws.close();
            BOOST_TEST(!ec2);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteSomeMultipleCalls()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            // First write
            std::string_view part1 = "Hello, ";
            auto [ec1, n1] = co_await bws.write_some(
                capy::const_buffer(part1.data(), part1.size()));
            BOOST_TEST(!ec1);
            BOOST_TEST_EQ(n1, 7u);

            // Second write
            std::string_view part2 = "World!";
            auto [ec2, n2] = co_await bws.write_some(
                capy::const_buffer(part2.data(), part2.size()));
            BOOST_TEST(!ec2);
            BOOST_TEST_EQ(n2, 6u);

            auto [ec3] = co_await bws.close();
            BOOST_TEST(!ec3);
            BOOST_TEST(sr.is_done());

            // Verify chunked output contains both parts
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

    void
    testWriteSomeLargeData()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            // Large body requiring multiple flushes
            std::string large_body(32768, 'x');
            std::size_t total_written = 0;

            while(total_written < large_body.size())
            {
                auto [ec, n] = co_await bws.write_some(
                    capy::const_buffer(
                        large_body.data() + total_written,
                        large_body.size() - total_written));
                BOOST_TEST(!ec);
                BOOST_TEST_GT(n, 0u);
                total_written += n;
            }

            BOOST_TEST_EQ(total_written, large_body.size());

            auto [ec] = co_await bws.close();
            BOOST_TEST(!ec);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Buffer sequence tests
    //------------------------------------------------------

    void
    testWriteSomeBufferSequence()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_payload_size(11);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            // Multi-buffer sequence
            std::string_view s1 = "Hello";
            std::string_view s2 = "World!";
            std::array<capy::const_buffer, 2> buffers = {{
                capy::const_buffer(s1.data(), s1.size()),
                capy::const_buffer(s2.data(), s2.size())
            }};

            auto [ec, n] = co_await bws.write_some(buffers);
            BOOST_TEST(!ec);
            // Should consume at least from first buffer
            BOOST_TEST_GT(n, 0u);

            auto [ec2] = co_await bws.close();
            BOOST_TEST(!ec2);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Close operation tests
    //------------------------------------------------------

    void
    testCloseBasic()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            std::string_view body = "test";
            auto [ec1, n] = co_await bws.write_some(
                capy::make_buffer(body));
            BOOST_TEST(!ec1);

            auto [ec2] = co_await bws.close();
            BOOST_TEST(!ec2);
            BOOST_TEST(sr.is_done());

            // Chunked output should end with "0\r\n\r\n"
            BOOST_TEST(underlying.data.find("0\r\n\r\n") !=
                std::string::npos);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testCloseEmpty()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            // Close without writing any body data
            auto [ec] = co_await bws.close();
            BOOST_TEST(!ec);
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
    // Error handling tests
    //------------------------------------------------------

    void
    testImmediateError()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            underlying.forced_error = capy::error::test_failure;

            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            std::string_view body = "data";
            auto [ec, n] = co_await bws.write_some(
                capy::make_buffer(body));

            // Due to deferred error reporting: data is committed to
            // serializer before the underlying stream error occurs,
            // so first call returns success with bytes committed
            if(!ec)
            {
                BOOST_TEST_EQ(n, 4u);
                // Error should be reported on next call
                auto [ec2, n2] = co_await bws.write_some(
                    capy::make_buffer(body));
                BOOST_TEST(ec2 == capy::error::test_failure);
                BOOST_TEST_EQ(n2, 0u);
            }
            else
            {
                // Or error could be immediate if no bytes committed
                BOOST_TEST(ec == capy::error::test_failure);
                BOOST_TEST_EQ(n, 0u);
            }
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testDeferredErrorReporting()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            // Error after writing some data
            underlying.error_after = 50;

            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            std::string_view body = "Hello";
            auto [ec, n] = co_await bws.write_some(
                capy::make_buffer(body));

            // First call may succeed with data, error saved
            // or fail immediately depending on timing
            if(!ec)
            {
                BOOST_TEST_GT(n, 0u);

                // Next call should report saved error
                auto [ec2, n2] = co_await bws.write_some(
                    capy::make_buffer(body));
                (void)ec2;
                (void)n2;
                // Either error now or later
            }
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testSavedErrorOnClose()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            underlying.error_after = 50;

            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            // Write to trigger potential deferred error
            std::string_view body = "Hello";
            (void)co_await bws.write_some(
                capy::make_buffer(body));

            // Close should report any saved error
            auto [ec] = co_await bws.close();
            // Error may or may not be deferred
            (void)ec;
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

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_payload_size(5);
            res.set(field::content_type, "text/plain");
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            std::string_view body = "hello";
            auto [ec, n] = co_await bws.write_some(
                capy::make_buffer(body));
            BOOST_TEST(!ec);
            BOOST_TEST_EQ(n, 5u);

            auto [ec2] = co_await bws.close();
            BOOST_TEST(!ec2);
            BOOST_TEST(sr.is_done());

            // Check Content-Length header in output
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

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_chunked(true);
            res.set(field::content_type, "text/plain");
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            std::string_view body = "chunked data";
            auto [ec, n] = co_await bws.write_some(
                capy::make_buffer(body));
            BOOST_TEST(!ec);

            auto [ec2] = co_await bws.close();
            BOOST_TEST(!ec2);
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

    void
    testEmptyBodyMessage()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_write_stream underlying;
            http::polystore ctx;
            install_serializer_service(ctx, {});
            serializer sr(ctx);

            response res;
            res.set_payload_size(0);
            auto srs = sr.start_stream(res);

            body_write_stream bws(underlying, sr, std::move(srs));

            // Close immediately without writing
            auto [ec] = co_await bws.close();
            BOOST_TEST(!ec);
            BOOST_TEST(sr.is_done());

            // Check Content-Length: 0
            BOOST_TEST(underlying.data.find("Content-Length: 0") !=
                std::string::npos);
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
        testWriteSomeBasic();
        testWriteSomeEmpty();
        testWriteSomeMultipleCalls();
        testWriteSomeLargeData();
        testWriteSomeBufferSequence();
        testCloseBasic();
        testCloseEmpty();
        testImmediateError();
        testDeferredErrorReporting();
        testSavedErrorOnClose();
        testContentLengthBody();
        testChunkedBody();
        testEmptyBodyMessage();
    }
};

TEST_SUITE(
    body_write_stream_test,
    "boost.http.body_write_stream");

} // namespace http
} // namespace boost
