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
#include <boost/http/request.hpp>

#include <boost/http/message_base.hpp>

#include <utility>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct request_test
{
    void
    testHelpers()
    {
        core::string_view const cs =
            "POST /x HTTP/1.0\r\n"
            "User-Agent: boost\r\n"
            "\r\n";
        request req(cs);
        BOOST_TEST(req.method() == method::post);
        BOOST_TEST(req.method_text() == "POST");
        BOOST_TEST(req.target() == "/x");
        BOOST_TEST(req.version() == version::http_1_0);
        BOOST_TEST(req.buffer().data() != cs.data());
        BOOST_TEST(req.buffer() == cs);
    }

    void
    testSpecial()
    {
        auto const check =
        []( request& req,
            std::size_t count,
            core::string_view s)
        {
            req = request(s);
            BOOST_TEST(
                req.size() == count);
        };

        core::string_view const cs =
            "POST /x HTTP/1.0\r\n"
            "Content-Length: 42\r\n"
            "User-Agent: boost\r\n"
            "\r\n";

        core::string_view const cs2 =
            "CONNECT 127.0.0.1 HTTP/1.1\r\n"
            "User-Agent: boost\r\n"
            "\r\n";

        // request()
        {
            request req;
            check(req, 0,
                "GET / HTTP/1.1\r\n"
                "\r\n");
            BOOST_TEST(
                req.method() == method::get);
            BOOST_TEST(
                req.method_text() == "GET");
            BOOST_TEST(
                req.target() == "/");
            BOOST_TEST(
                req.version() == version::http_1_1);
        }

        // request(request const&)
        {
            {
                // default
                request r1;
                request r2(r1);
                check(r2, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                BOOST_TEST(
                    r2.method() == method::get);
                BOOST_TEST(
                    r2.method_text() == "GET");
                BOOST_TEST(
                    r2.version() == version::http_1_1);
            }
            {
                request r1(cs);
                request r2(r1);
                check(r1, 2, cs);
                check(r2, 2, cs);
                BOOST_TEST(
                    r2.buffer().data() !=
                        r1.buffer().data());
                BOOST_TEST(
                    r2.method() == method::post);
                BOOST_TEST(
                    r2.method_text() == "POST");
                BOOST_TEST(
                    r2.version() == version::http_1_0);
            }
        }

        // request(request const&)
        {
            {
                // default
                request_base const& r1 = request();
                request r2(r1);
                check(r2, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                BOOST_TEST(
                    r2.method() == method::get);
                BOOST_TEST(
                    r2.method_text() == "GET");
                BOOST_TEST(
                    r2.version() == version::http_1_1);
            }
            {
                request_base const& r1 = request(cs);
                request r2(r1);
                check(r2, 2, cs);
                BOOST_TEST(
                    r2.buffer().data() !=
                        r1.buffer().data());
                BOOST_TEST(
                    r2.method() == method::post);
                BOOST_TEST(
                    r2.method_text() == "POST");
                BOOST_TEST(
                    r2.version() == version::http_1_0);
            }
        }

        // request(request&&)
        {
            {
                // default
                request r1;
                request r2(std::move(r1));
                check(r1, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                check(r2, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                BOOST_TEST(
                    r2.method() == method::get);
                BOOST_TEST(
                    r2.method_text() == "GET");
                BOOST_TEST(
                    r2.version() == version::http_1_1);
            }
            {
                request r1(cs);
                request r2(std::move(r1));
                check(r1, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                check(r2, 2, cs);
                BOOST_TEST(
                    r2.buffer().data() !=
                        r1.buffer().data());
                BOOST_TEST(
                    r2.method() == method::post);
                BOOST_TEST(
                    r2.method_text() == "POST");
                BOOST_TEST(
                    r2.version() == version::http_1_0);
            }
        }

        // operator=(request const&)
        {
            {
                // default
                request r1;
                request r2(cs);
                r1 = r2;
                check(r1, 2, cs);
                check(r2, 2, cs);
                BOOST_TEST(
                    r2.buffer().data() !=
                        r1.buffer().data());
                BOOST_TEST(
                    r1.method() == method::post);
                BOOST_TEST(
                    r1.method_text() == "POST");
                BOOST_TEST(
                    r1.version() == version::http_1_0);
            }
            {
                request r1(cs);
                request r2;
                r1 = r2;
                check(r1, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                BOOST_TEST(
                    r1.buffer().data() !=
                        r2.buffer().data());
                BOOST_TEST(
                    r1.method() == method::get);
                BOOST_TEST(
                    r1.method_text() == "GET");
                BOOST_TEST(
                    r1.version() == version::http_1_1);
            }
        }

        // operator=(request_base const&)
        {
            {
                request r1(cs);
                request_base const& r2 = request();
                r1 = r2;
                check(r1, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                BOOST_TEST(
                    r1.buffer().data() !=
                        r2.buffer().data());
                BOOST_TEST(
                    r1.method() == method::get);
                BOOST_TEST(
                    r1.method_text() == "GET");
                BOOST_TEST(
                    r1.version() == version::http_1_1);
            }
        }

        // operator=(fields&&)
        {
            {
                request r1(cs);
                request r2;
                r2 = std::move(r1);
                check(r1, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                check(r2, 2, cs);
                BOOST_TEST(
                    r2.method() == method::post);
                BOOST_TEST(
                    r2.method_text() == "POST");
                BOOST_TEST(
                    r2.target() == "/x");
                BOOST_TEST(
                    r2.version() == version::http_1_0);
            }
            {
                request r1(cs);
                request r2(cs2);
                r2 = std::move(r1);
                check(r1, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                check(r2, 2, cs);
                BOOST_TEST(
                    r2.method() == method::post);
                BOOST_TEST(
                    r2.method_text() == "POST");
                BOOST_TEST(
                    r2.target() == "/x");
                BOOST_TEST(
                    r2.version() == version::http_1_0);
            }
            {
                request r1;
                request r2(cs);
                r2 = std::move(r1);
                check(r1, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                check(r2, 0,
                    "GET / HTTP/1.1\r\n"
                    "\r\n");
                BOOST_TEST(
                    r1.buffer().data() !=
                    r2.buffer().data());
                BOOST_TEST(
                    r2.method() == method::get);
                BOOST_TEST(
                    r2.method_text() == "GET");
                BOOST_TEST(
                    r2.version() == version::http_1_1);
            }
        }
    }

    void
    testObservers()
    {
    }

    void
    testModifiers()
    {
        // clear()
        {
            {
                request req;
                BOOST_TEST(req.capacity_in_bytes() == 0);
                req.clear();
                BOOST_TEST(req.capacity_in_bytes() == 0);
                BOOST_TEST(req.buffer() ==
                    "GET / HTTP/1.1\r\n\r\n");
            }
            {
                request req(
                    "POST /x HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                auto const n =
                    req.capacity_in_bytes();
                BOOST_TEST(n > 0);
                req.clear();
                BOOST_TEST(req.size() == 0);
                BOOST_TEST(
                    req.capacity_in_bytes() == n);
                BOOST_TEST(
                    req.method() == method::get);
                BOOST_TEST(
                    req.method_text() == "GET");
                BOOST_TEST(
                    req.target() == "/");
                BOOST_TEST(
                    req.version() == version::http_1_1);
                BOOST_TEST(req.buffer() ==
                    "GET / HTTP/1.1\r\n\r\n");
            }
        }

        // set_method(method)
        {
            {
                request req;
                req.set_method(method::delete_);
                BOOST_TEST(
                    req.method() == method::delete_);
                BOOST_TEST(
                    req.method_text() == "DELETE");
                BOOST_TEST(req.buffer() ==
                    "DELETE / HTTP/1.1\r\n\r\n");
            }
            {
                request req(
                    "POST /x HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_method(method::delete_);
                BOOST_TEST(
                    req.method() == method::delete_);
                BOOST_TEST(
                    req.method_text() == "DELETE");
                BOOST_TEST(req.buffer() ==
                    "DELETE /x HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
        }

        // set_method(string_view)
        {
            {
                request req;
                req.set_method("DELETE");
                BOOST_TEST(
                    req.method() == method::delete_);
                BOOST_TEST(
                    req.method_text() == "DELETE");
                BOOST_TEST(req.buffer() ==
                    "DELETE / HTTP/1.1\r\n\r\n");
            }
            {
                request req(
                    "POST /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_method("DELETE");
                BOOST_TEST(
                    req.method() == method::delete_);
                BOOST_TEST(
                    req.method_text() == "DELETE");
                BOOST_TEST(req.buffer() ==
                    "DELETE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
            {
                request req(
                    "DELETE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_method("PUT");
                BOOST_TEST(
                    req.method() == method::put);
                BOOST_TEST(
                    req.method_text() == "PUT");
                BOOST_TEST(req.buffer() ==
                    "PUT /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
            {
                request req(
                    "SOMETHINGSUPERLONGHERE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_method("PUT");
                BOOST_TEST(
                    req.method() == method::put);
                BOOST_TEST(
                    req.method_text() == "PUT");
                BOOST_TEST_EQ(req.buffer(),
                    "PUT /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
            {
                request req(
                    "SOMETHINGSUPERLONGHERE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_method("SOMETHINGSUPERLONGHERE");
                BOOST_TEST(
                    req.method() == method::unknown);
                BOOST_TEST(
                    req.method_text() == "SOMETHINGSUPERLONGHERE");
                BOOST_TEST(req.buffer() ==
                    "SOMETHINGSUPERLONGHERE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
        }

        // set_target
        {
            {
                request req;
                req.set_target("/index.htm");
                BOOST_TEST(
                    req.target() == "/index.htm");
                BOOST_TEST(req.buffer() ==
                    "GET /index.htm HTTP/1.1\r\n\r\n");
            }
            {
                request req(
                    "POST /x HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_target("/index.htm");
                BOOST_TEST(req.buffer() ==
                    "POST /index.htm HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
            {
                // shrinks
                request req(
                    "SOMETHINGSUPERLONGHERE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_target("/abc");
                BOOST_TEST_EQ(
                    req.target(), "/abc");
                BOOST_TEST_EQ(req.buffer(),
                    "SOMETHINGSUPERLONGHERE /abc HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
            {
                // same size
                request req(
                    "SOMETHINGSUPERLONGHERE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_target("/zyxwvutsrqponmlkjihgfedcba");
                BOOST_TEST_EQ(
                    req.target(), "/zyxwvutsrqponmlkjihgfedcba");
                BOOST_TEST_EQ(req.buffer(),
                    "SOMETHINGSUPERLONGHERE /zyxwvutsrqponmlkjihgfedcba HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
            {
                // grows
                request req(
                    "SOMETHINGSUPERLONGHERE /abcdefghijklmnopqrstuvwxyz HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_target("/abcdefghijklmnopqrstuvwxyzzyxwvutsrqponmlkjihgfedcba");
                BOOST_TEST_EQ(
                    req.target(), "/abcdefghijklmnopqrstuvwxyzzyxwvutsrqponmlkjihgfedcba");
                BOOST_TEST_EQ(req.buffer(),
                    "SOMETHINGSUPERLONGHERE /abcdefghijklmnopqrstuvwxyzzyxwvutsrqponmlkjihgfedcba HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
        }

        // set_version
        {
            {
                request req;
                req.set_version(version::http_1_0);
                BOOST_TEST(
                    req.version() == version::http_1_0);
                BOOST_TEST(req.buffer() ==
                    "GET / HTTP/1.0\r\n\r\n");
            }
            {
                request req(
                    "POST /x HTTP/1.1\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
                req.set_version(version::http_1_0);
                BOOST_TEST(
                    req.version() == version::http_1_0);
                BOOST_TEST(req.buffer() ==
                    "POST /x HTTP/1.0\r\n"
                    "User-Agent: boost\r\n"
                    "\r\n");
            }
        }
    }

    void
    testExpect()
    {
        {
            request req;
            req.set_expect_100_continue(true);
            BOOST_TEST(
                req.metadata().expect.is_100_continue);

            req.set_expect_100_continue(false);
            BOOST_TEST(
                !req.metadata().expect.is_100_continue);
        }


        {
            request req;
            req.set_expect_100_continue(false);
            BOOST_TEST(
                !req.metadata().expect.is_100_continue);
        }

        {
            request req;
            req.set_expect_100_continue(true);
            BOOST_TEST(
                req.metadata().expect.is_100_continue);

            req.set_expect_100_continue(true);
            BOOST_TEST(
                req.metadata().expect.is_100_continue);
        }

        {
            request req;
            BOOST_TEST(
                !req.metadata().expect.ec.failed());

            req.set("Expect", "101-dalmations");
            BOOST_TEST(
                req.metadata().expect.ec.failed());

            req.set_expect_100_continue(true);
            BOOST_TEST(
                !req.metadata().expect.ec.failed());
            BOOST_TEST(
                req.metadata().expect.is_100_continue);
        }

        {
            request req;
            req.append("Expect", "100-continue");
            req.append("Expect", "101-dalmations");

            BOOST_TEST(
                req.metadata().expect.ec.failed());
            BOOST_TEST_EQ(
                req.count("Expect"), 2);

            req.set_expect_100_continue(true);
            BOOST_TEST(
                !req.metadata().expect.ec.failed());
            BOOST_TEST(
                req.metadata().expect.is_100_continue);
            BOOST_TEST_EQ(
                req.count("Expect"), 1);
        }

        {
            request req;
            req.append("Expect", "100-continue");
            req.append("Content-Length", "1234");
            req.append("Expect", "100-continue");

            BOOST_TEST(
                req.metadata().expect.ec.failed());
            BOOST_TEST_EQ(
                req.count("Expect"), 2);

            req.set_expect_100_continue(true);
            BOOST_TEST(
                !req.metadata().expect.ec.failed());
            BOOST_TEST(
                req.metadata().expect.is_100_continue);
            BOOST_TEST_EQ(
                req.count("Expect"), 1);
        }

        {
            request req;
            req.append("Expect", "404-not-found");
            req.append("Content-Length", "1234");
            req.append("Expect", "101-dalmations");

            BOOST_TEST(
                req.metadata().expect.ec.failed());
            BOOST_TEST_EQ(
                req.count("Expect"), 2);

            req.set_expect_100_continue(true);
            BOOST_TEST(
                !req.metadata().expect.ec.failed());
            BOOST_TEST(
                req.metadata().expect.is_100_continue);
            BOOST_TEST_EQ(
                req.count("Expect"), 1);
        }

        {
            request req;
            req.append("Expect", "100-continue");
            req.append("Content-Length", "1234");
            req.append("Expect", "100-continue");

            BOOST_TEST(
                req.metadata().expect.ec.failed());
            BOOST_TEST_EQ(
                req.count("Expect"), 2);

            req.set_expect_100_continue(false);
            BOOST_TEST(
                !req.metadata().expect.ec.failed());
            BOOST_TEST(
                !req.metadata().expect.is_100_continue);
            BOOST_TEST_EQ(
                req.count("Expect"), 0);
            BOOST_TEST_EQ(
                req.count("Content-Length"), 1);
        }
    }

    void
    testInitialSize()
    {
        auto check_default =[](request& f)
        {
            BOOST_TEST_EQ(
                f.capacity_in_bytes(), 0);

            auto const old = f.buffer().data();
            f.append(field::host, "www.google.com");
            f.append(field::connection, "close");
            f.insert(
                f.find(field::host),
                field::content_length, "1234");

            BOOST_TEST_NE(
                f.buffer().data(), old);
            // implies that the default capacity is larger than whatever is
            // required for these simple operations
            BOOST_TEST_GT(
                f.capacity_in_bytes(), 0);
            BOOST_TEST_GE(
                f.max_capacity_in_bytes(), f.capacity_in_bytes());
        };

        auto check = [](
            request& f,
            std::size_t size,
            std::size_t max_size)
        {
            auto const old = f.buffer().data();
            f.append(field::host, "www.google.com");
            f.append(field::connection, "close");
            f.insert(
                f.find(field::host),
                field::content_length, "1234");

            BOOST_TEST_EQ(
                f.buffer().data(), old);
            BOOST_TEST_EQ(
                f.capacity_in_bytes(), size);
            BOOST_TEST_EQ(
                f.max_capacity_in_bytes(), max_size);
            BOOST_TEST_THROWS(
                f.reserve_bytes(max_size + 1),
                std::length_error);
        };

        {
            request f;
            check_default(f);
        }

        {
            request f(0);
            check_default(f);
        }

        {
            request f(0, 0);
            BOOST_TEST_THROWS(
                f.append(field::host, "www.google.com"),
                std::length_error);
            BOOST_TEST_EQ(
                f.max_capacity_in_bytes(), 0);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = init;

            request f(init, cap);
            check(f, init, cap);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = 8192;

            request f(init, cap);
            check(f, init, cap);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = 8192;

            request f(init, cap);
            request f2(2 * init, 2 * cap);
            check(f, init, cap);

            f = f2;
            // check(f, init, 2 * cap);
            // check(f2, 2 * init, 2 * cap);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = 8192;

            request f(init, cap);
            request f2(2 * init, 2 * cap);
            check(f, init, cap);

            f = std::move(f2);
            check(f, 2 * init, 2 * cap);
        }

        {
            BOOST_TEST_THROWS(
                request(1024, 0), std::logic_error);

            BOOST_TEST_THROWS(
                request(1024, 512), std::logic_error);
        }
    }

    void
    run()
    {
        testHelpers();
        testSpecial();
        testObservers();
        testModifiers();
        testExpect();
        testInitialSize();
    }
};

TEST_SUITE(
    request_test,
    "boost.http.request");

} // http
} // boost
