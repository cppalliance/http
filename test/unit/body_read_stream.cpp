//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/body_read_stream.hpp>

#include <boost/http/request_parser.hpp>
#include <boost/http/response_parser.hpp>
#include <boost/http/core/polystore.hpp>

#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/copy.hpp>
#include <boost/capy/cond.hpp>
#include <boost/capy/concept/read_stream.hpp>
#include <boost/capy/error.hpp>
#include <boost/capy/ex/execution_context.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>

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
// Mock ReadStream for testing
//----------------------------------------------------------

struct mock_read_stream
{
    std::string data;
    std::size_t pos = 0;
    std::size_t chunk_size = 65536;
    system::error_code forced_error;
    std::size_t error_after = std::size_t(-1);
    std::size_t total_read = 0;

    template<capy::MutableBufferSequence MB>
    auto read_some(MB const& buffers)
    {
        struct awaitable
        {
            mock_read_stream* self_;
            capy::mutable_buffer buf_;

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

                if(self_->total_read >= self_->error_after)
                    return {capy::error::test_failure, 0};

                if(self_->pos >= self_->data.size())
                    return {capy::error::eof, 0};

                std::size_t available = self_->data.size() - self_->pos;
                std::size_t can_read = self_->error_after - self_->total_read;
                std::size_t to_read = std::min({
                    buf_.size(),
                    available,
                    can_read,
                    self_->chunk_size});

                std::memcpy(
                    buf_.data(),
                    self_->data.data() + self_->pos,
                    to_read);

                self_->pos += to_read;
                self_->total_read += to_read;
                return {{}, to_read};
            }
        };

        capy::mutable_buffer buf = *capy::begin(buffers);
        return awaitable{this, buf};
    }
};

static_assert(capy::ReadStream<mock_read_stream>);

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
// body_read_stream_test
//----------------------------------------------------------

struct body_read_stream_test
{
    // Verify body_read_stream satisfies ReadStream concept
    static_assert(capy::ReadStream<
        body_read_stream<mock_read_stream>>);

    //------------------------------------------------------
    // Basic functionality tests
    //------------------------------------------------------

    void
    testReadSomeBasic()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "Hello, World!";

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            char buf[64];
            auto [ec, n] = co_await brs.read_some(
                capy::mutable_buffer(buf, sizeof(buf)));

            BOOST_TEST(!ec);
            BOOST_TEST_EQ(n, 13u);
            BOOST_TEST(std::string_view(buf, n) == "Hello, World!");
            BOOST_TEST(pr.is_complete());
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testReadSomeEmpty()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 5\r\n"
                "\r\n"
                "hello";

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            // Empty buffer should complete immediately with n=0
            auto [ec, n] = co_await brs.read_some(
                capy::mutable_buffer(nullptr, 0));

            BOOST_TEST(!ec);
            BOOST_TEST_EQ(n, 0u);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testReadSomeMultipleCalls()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 26\r\n"
                "\r\n"
                "abcdefghijklmnopqrstuvwxyz";
            underlying.chunk_size = 10;

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            std::string body;
            char buf[8];

            while(true)
            {
                auto [ec, n] = co_await brs.read_some(
                    capy::mutable_buffer(buf, sizeof(buf)));

                if(ec == capy::cond::eof)
                    break;

                BOOST_TEST(!ec);
                body.append(buf, n);
            }

            BOOST_TEST_EQ(body.size(), 26u);
            BOOST_TEST(body == "abcdefghijklmnopqrstuvwxyz");
            BOOST_TEST(pr.is_complete());
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testReadSomeLargeBody()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            std::string large_body(8192, 'x');

            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 8192\r\n"
                "\r\n" + large_body;
            underlying.chunk_size = 1024;

            http::polystore ctx;
            response_parser::config cfg;
            cfg.body_limit = 16384;
            install_parser_service(ctx, cfg);
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            std::string body;
            char buf[2048];

            while(true)
            {
                auto [ec, n] = co_await brs.read_some(
                    capy::mutable_buffer(buf, sizeof(buf)));

                if(ec == capy::cond::eof)
                    break;

                BOOST_TEST(!ec);
                body.append(buf, n);
            }

            BOOST_TEST_EQ(body.size(), 8192u);
            BOOST_TEST(body == large_body);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Chunked encoding tests
    //------------------------------------------------------

    void
    testChunkedBody()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "5\r\n"
                "Hello\r\n"
                "7\r\n"
                ", World\r\n"
                "0\r\n"
                "\r\n";

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            std::string body;
            char buf[64];

            while(true)
            {
                auto [ec, n] = co_await brs.read_some(
                    capy::mutable_buffer(buf, sizeof(buf)));

                if(ec == capy::cond::eof)
                    break;

                BOOST_TEST(!ec);
                body.append(buf, n);
            }

            BOOST_TEST(body == "Hello, World");
            BOOST_TEST(pr.is_complete());
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // EOF tests
    //------------------------------------------------------

    void
    testEofOnComplete()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 4\r\n"
                "\r\n"
                "test";

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            char buf[64];

            // First read gets the body
            auto [ec1, n1] = co_await brs.read_some(
                capy::mutable_buffer(buf, sizeof(buf)));
            BOOST_TEST(!ec1);
            BOOST_TEST_EQ(n1, 4u);

            // Second read should return EOF
            auto [ec2, n2] = co_await brs.read_some(
                capy::mutable_buffer(buf, sizeof(buf)));
            BOOST_TEST(ec2 == capy::cond::eof);
            BOOST_TEST_EQ(n2, 0u);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testEmptyBody()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 0\r\n"
                "\r\n";

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            char buf[64];
            auto [ec, n] = co_await brs.read_some(
                capy::mutable_buffer(buf, sizeof(buf)));

            BOOST_TEST(ec == capy::cond::eof);
            BOOST_TEST_EQ(n, 0u);
            BOOST_TEST(pr.is_complete());
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
            mock_read_stream underlying;
            underlying.forced_error = capy::error::test_failure;

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            char buf[64];
            auto [ec, n] = co_await brs.read_some(
                capy::mutable_buffer(buf, sizeof(buf)));

            BOOST_TEST(ec == capy::error::test_failure);
            BOOST_TEST_EQ(n, 0u);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testErrorDuringBody()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            // Create a body larger than error_after to trigger the error
            std::string partial_body(100, 'x');

            mock_read_stream underlying;
            underlying.data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 200\r\n"
                "\r\n" + partial_body;
            underlying.chunk_size = 10;
            // Error after reading header (~40 bytes) + ~10 bytes of body
            underlying.error_after = 50;

            http::polystore ctx;
            install_parser_service(ctx, response_parser::config{});
            response_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            std::string body;
            char buf[64];
            system::error_code last_ec;

            while(true)
            {
                auto [ec, n] = co_await brs.read_some(
                    capy::mutable_buffer(buf, sizeof(buf)));

                last_ec = ec;
                if(ec && ec != capy::cond::eof)
                    break;
                if(ec == capy::cond::eof)
                    break;

                body.append(buf, n);
            }

            // Should have an error (test_failure)
            BOOST_TEST(last_ec == capy::error::test_failure);
            // Should have read some data before error
            BOOST_TEST(body.size() > 0);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Request parser tests
    //------------------------------------------------------

    void
    testRequestParser()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            mock_read_stream underlying;
            underlying.data =
                "POST /upload HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "Content-Length: 11\r\n"
                "\r\n"
                "Hello World";

            http::polystore ctx;
            install_parser_service(ctx, request_parser::config{});
            request_parser pr(ctx);
            pr.reset();
            pr.start();

            body_read_stream brs(underlying, pr);

            char buf[64];
            auto [ec, n] = co_await brs.read_some(
                capy::mutable_buffer(buf, sizeof(buf)));

            BOOST_TEST(!ec);
            BOOST_TEST_EQ(n, 11u);
            BOOST_TEST(std::string_view(buf, n) == "Hello World");
            BOOST_TEST(pr.is_complete());

            // Verify header was parsed
            BOOST_TEST(pr.got_header());
            auto const& req = pr.get();
            BOOST_TEST(req.method() == method::post);
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
        testReadSomeBasic();
        testReadSomeEmpty();
        testReadSomeMultipleCalls();
        testReadSomeLargeBody();
        testChunkedBody();
        testEofOnComplete();
        testEmptyBody();
        testImmediateError();
        testErrorDuringBody();
        testRequestParser();
    }
};

TEST_SUITE(
    body_read_stream_test,
    "boost.http.body_read_stream");

} // namespace http
} // namespace boost
