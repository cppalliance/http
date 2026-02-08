//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/static_request.hpp>

#include <boost/http/request.hpp>

#include <utility>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct static_request_test
{
    void
    testSpecial()
    {
        core::string_view const cs =
            "POST /x HTTP/1.0\r\n"
            "Content-Length: 42\r\n"
            "User-Agent: boost\r\n"
            "\r\n";

        // static_request(void* storage, std::size_t cap)
        {
            char buf[64];
            static_request req(buf, sizeof(buf));
            BOOST_TEST(
                req.method() == method::get);
            BOOST_TEST(
                req.method_text() == "GET");
            BOOST_TEST(
                req.target() == "/");
            BOOST_TEST(
                req.version() == version::http_1_1);
        }

        // static_request(static_request&&)
        {
            char buf[64];
            static_request r1(buf, sizeof(buf));
            static_request r2(std::move(r1));
            BOOST_TEST(
                r2.buffer().data() != r1.buffer().data());
            BOOST_TEST(
                r2.buffer().data() == buf);
        }

        // operator=(request_base const&)
        {
            char buf1[128];
            static_request r1(buf1, sizeof(buf1));
            const request r2(cs);
            r1 = r2;
            BOOST_TEST(
                r1.buffer() == cs);
            BOOST_TEST(
                r1.buffer().data() != r2.buffer().data());
        }

        // operator=(static_request const&)
        {
            char buf1[128];
            static_request r1(buf1, sizeof(buf1));
            r1 = request(cs);
            char buf2[128];
            static_request r2(buf2, sizeof(buf2));
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
        char buf[96];
        static_request r(buf, sizeof(buf));

        BOOST_TEST_EQ(
            r.capacity_in_bytes(), 96);
        BOOST_TEST_EQ(
            r.max_capacity_in_bytes(), 96);

        r.append("T", "*");
        r.append("T", "*");
        r.append("T", "*");

        BOOST_TEST_THROWS(
            r.append("T", "*"),
            std::length_error);

        BOOST_TEST_THROWS(
            r.set_target("/index.html"),
            std::length_error);

        r.set_target("/");
    }

    void
    run()
    {
        testSpecial();
        testCapacity();
    }
};

TEST_SUITE(
    static_request_test,
    "boost.http.static_request");

} // http
} // boost
