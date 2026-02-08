//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/static_response.hpp>

#include <boost/http/response.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

class static_response_test
{
public:
    void
    testSpecial()
    {
        core::string_view const cs =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Server: test\r\n"
            "\r\n";

        // static_response(void* storage, std::size_t cap)
        {
            char buf[64];
            static_response req(buf, sizeof(buf));
            BOOST_TEST(
                req.status_int() == 200);
            BOOST_TEST(
                req.status() == status::ok);
            BOOST_TEST(
                req.version() == version::http_1_1);
        }

        // static_response(static_response&&)
        {
            char buf[64];
            static_response r1(buf, sizeof(buf));
            static_response r2(std::move(r1));
            BOOST_TEST(
                r2.buffer().data() != r1.buffer().data());
            BOOST_TEST(
                r2.buffer().data() == buf);
        }

        // operator=(response_base const&)
        {
            char buf1[128];
            static_response r1(buf1, sizeof(buf1));
            const response r2(cs);
            r1 = r2;
            BOOST_TEST(
                r1.buffer() == cs);
            BOOST_TEST(
                r1.buffer().data() != r2.buffer().data());
        }

        // operator=(static_response const&)
        {
            char buf1[128];
            static_response r1(buf1, sizeof(buf1));
            r1 = response(cs);
            char buf2[128];
            static_response r2(buf2, sizeof(buf2));
            r2 = r1;
            BOOST_TEST(
                r2.buffer() == cs);
            BOOST_TEST(
                r2.buffer().data() != r1.buffer().data());
        }

    }

    void
    testCapacity()
    {
        char buf[32];
        static_response f(buf, sizeof(buf));
        BOOST_TEST_THROWS(
            f.append(field::host, "www.google.com"),
            std::length_error);
        BOOST_TEST_EQ(
            f.capacity_in_bytes(), 32);
        BOOST_TEST_EQ(
            f.max_capacity_in_bytes(), 32);
    }

    void
    run()
    {
        testSpecial();
        testCapacity();
    }
};

TEST_SUITE(
    static_response_test,
    "boost.http.static_response");

} // http
} // boost
