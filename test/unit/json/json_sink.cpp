//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/json/json_sink.hpp>

#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/concept/write_sink.hpp>
#include <boost/capy/ex/execution_context.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/json/value.hpp>

#include "../test_helpers.hpp"

#include <string_view>

namespace boost {
namespace http {

namespace {

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
// json_sink_test
//----------------------------------------------------------

struct json_sink_test
{
    // Verify json_sink satisfies WriteSink concept
    static_assert(capy::WriteSink<json_sink>);

    //------------------------------------------------------
    // Basic functionality tests
    //------------------------------------------------------

    void
    testWriteBasic()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            std::string_view data = R"({"key": "value"})";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(data));

            BOOST_TEST(!ec.failed());
            BOOST_TEST_EQ(n, data.size());

            auto [ec2] = co_await sink.write_eof();
            BOOST_TEST(!ec2.failed());
            BOOST_TEST(sink.done());

            auto v = sink.release();
            BOOST_TEST(v.is_object());
            BOOST_TEST_EQ(v.at("key").as_string(), "value");
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

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            std::string_view data = R"(42)";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(data), true);

            BOOST_TEST(!ec.failed());
            BOOST_TEST_EQ(n, data.size());
            BOOST_TEST(sink.done());

            auto v = sink.release();
            BOOST_TEST(v.is_int64());
            BOOST_TEST_EQ(v.as_int64(), 42);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteIncremental()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            // Write JSON in parts
            std::string_view part1 = R"({"name":)";
            std::string_view part2 = R"("test",)";
            std::string_view part3 = R"("count":123})";

            auto [ec1, n1] = co_await sink.write(
                capy::make_buffer(part1));
            BOOST_TEST(!ec1.failed());
            BOOST_TEST_EQ(n1, part1.size());
            BOOST_TEST(!sink.done());

            auto [ec2, n2] = co_await sink.write(
                capy::make_buffer(part2));
            BOOST_TEST(!ec2.failed());
            BOOST_TEST_EQ(n2, part2.size());
            BOOST_TEST(!sink.done());

            auto [ec3, n3] = co_await sink.write(
                capy::make_buffer(part3), true);
            BOOST_TEST(!ec3.failed());
            BOOST_TEST_EQ(n3, part3.size());
            BOOST_TEST(sink.done());

            auto v = sink.release();
            BOOST_TEST(v.is_object());
            BOOST_TEST_EQ(v.at("name").as_string(), "test");
            BOOST_TEST_EQ(v.at("count").as_int64(), 123);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteArray()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            std::string_view data = R"([1, 2, 3, "four"])";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(data), true);

            BOOST_TEST(!ec.failed());
            BOOST_TEST(sink.done());

            auto v = sink.release();
            BOOST_TEST(v.is_array());
            BOOST_TEST_EQ(v.as_array().size(), 4u);
            BOOST_TEST_EQ(v.at(0).as_int64(), 1);
            BOOST_TEST_EQ(v.at(3).as_string(), "four");
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

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            // Empty buffer write should succeed
            auto [ec, n] = co_await sink.write(
                capy::const_buffer(nullptr, 0));

            BOOST_TEST(!ec.failed());
            BOOST_TEST_EQ(n, 0u);
            BOOST_TEST(!sink.done());
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
    testWriteInvalidJson()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            std::string_view data = R"({invalid json})";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(data), true);

            BOOST_TEST(ec.failed());
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    void
    testWriteIncompleteJson()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            // Incomplete JSON
            std::string_view data = R"({"key":)";
            auto [ec1, n] = co_await sink.write(
                capy::make_buffer(data));
            BOOST_TEST(!ec1.failed());

            // Calling write_eof with incomplete JSON should error
            auto [ec2] = co_await sink.write_eof();
            BOOST_TEST(ec2.failed());
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Constructor tests
    //------------------------------------------------------

    void
    testConstructWithOptions()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            json::parse_options opt;
            opt.allow_comments = true;
            opt.allow_trailing_commas = true;

            json_sink sink(opt);

            // JSON with comments and trailing comma
            std::string_view data = R"({
                // This is a comment
                "key": "value",
            })";
            auto [ec, n] = co_await sink.write(
                capy::make_buffer(data), true);

            BOOST_TEST(!ec.failed());
            BOOST_TEST(sink.done());

            auto v = sink.release();
            BOOST_TEST(v.is_object());
            BOOST_TEST_EQ(v.at("key").as_string(), "value");
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Reset test
    //------------------------------------------------------

    void
    testReset()
    {
        int dispatch_count = 0;
        test_executor ex(dispatch_count);
        bool completed = false;

        auto do_test = []() -> capy::task<void>
        {
            json_sink sink;

            // First parse
            std::string_view data1 = R"({"first": 1})";
            auto [ec1, n1] = co_await sink.write(
                capy::make_buffer(data1), true);
            BOOST_TEST(!ec1.failed());
            BOOST_TEST(sink.done());

            auto v1 = sink.release();
            BOOST_TEST_EQ(v1.at("first").as_int64(), 1);

            // Reset and parse again
            sink.reset();
            BOOST_TEST(!sink.done());

            std::string_view data2 = R"({"second": 2})";
            auto [ec2, n2] = co_await sink.write(
                capy::make_buffer(data2), true);
            BOOST_TEST(!ec2.failed());
            BOOST_TEST(sink.done());

            auto v2 = sink.release();
            BOOST_TEST_EQ(v2.at("second").as_int64(), 2);
        };

        capy::run_async(ex,
            [&]() { completed = true; },
            [](std::exception_ptr) {})(do_test());

        BOOST_TEST(completed);
    }

    //------------------------------------------------------
    // Parser access test
    //------------------------------------------------------

    void
    run()
    {
        testWriteBasic();
        testWriteWithEof();
        testWriteIncremental();
        testWriteArray();
        testWriteEmpty();
        testWriteInvalidJson();
        testWriteIncompleteJson();
        testConstructWithOptions();
        testReset();
    }
};

TEST_SUITE(
    json_sink_test,
    "boost.http.json.json_sink");

} // namespace http
} // namespace boost
