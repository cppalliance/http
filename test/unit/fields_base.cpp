//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/fields_base.hpp>

#include <boost/http/field.hpp>
#include <boost/http/fields.hpp>
#include <boost/http/request.hpp>
#include <boost/http/response.hpp>
#include <boost/http/error.hpp>

#include <boost/optional/optional_io.hpp>
#include <boost/static_assert.hpp>

#include "test_helpers.hpp"
#include "test_suite.hpp"

#include <stdexcept>
#include <vector>

namespace boost {
namespace http {

// check for overflow
//BOOST_STATIC_ASSERT(
    //fields_base::max_capacity_in_bytes() >= max_offset);

struct fields_base_test
{
    static
    void
    check(
        core::string_view s0,
        void(*fn)(fields_base&),
        core::string_view s1)
    {
        // fields
        {
            fields f(s0);
            fn(f);
            BOOST_TEST_EQ(f.buffer(), s1);
            test_fields(f, s1);
        }

        // request
        {
            auto const m = std::string() +
                "GET / HTTP/1.1\r\n" +
                std::string(s0);
            auto const m1 = std::string() +
                "GET / HTTP/1.1\r\n" +
                std::string(s1);
            request req(m);
            fn(req);
            BOOST_TEST_EQ(req.buffer(), m1);
            test_fields(req, s1);
        }

        // response
        {
            auto const m = std::string() +
                "HTTP/1.1 200 OK\r\n" +
                std::string(s0);
            auto const m1 = std::string() +
                "HTTP/1.1 200 OK\r\n" +
                std::string(s1);
            response res(m);
            fn(res);
            BOOST_TEST_EQ(res.buffer(), m1);
            test_fields(res, s1);
        }
    }

    static
    void
    check_error(
        core::string_view s0,
        void(*fn)(fields_base&))
    {
        // fields
        {
            fields f(s0);
            fn(f);
            BOOST_TEST_EQ(f.buffer(), s0);
        }
    }

    void
    testCapacity()
    {
        // clear()
        {
            // default fields
            {
                fields f;
                f.clear();
                BOOST_TEST_EQ(
                    f.buffer(),
                    "\r\n");
                BOOST_TEST_EQ(
                    f.capacity_in_bytes(), 0);
            }

            // default request
            {
                request req;
                req.clear();
                BOOST_TEST_EQ(
                    req.buffer(),
                    "GET / HTTP/1.1\r\n\r\n");
                BOOST_TEST_EQ(
                    req.capacity_in_bytes(), 0);
            }

            // default response
            {
                response res;
                res.clear();
                BOOST_TEST_EQ(
                    res.buffer(),
                    "HTTP/1.1 200 OK\r\n\r\n");
                BOOST_TEST_EQ(
                    res.capacity_in_bytes(), 0);
            }

            {
                fields f("\r\n");
                auto const n =
                    f.capacity_in_bytes();
                BOOST_TEST_GT(n, 0);
                BOOST_TEST_EQ(
                    f.buffer(),
                    "\r\n");
            }

            {
                BOOST_TEST_THROWS(
                    fields("HTTP/1.1"),
                    std::invalid_argument);
            }

            {
                fields f(
                    "digest: ffce\r\n"
                    "type: 3\r\n"
                    "\r\n");
                auto const n =
                    f.capacity_in_bytes();
                BOOST_TEST_GT(n, 0);
                f.clear();
                BOOST_TEST_EQ(
                    f.buffer(),
                    "\r\n");
                BOOST_TEST_EQ(
                    f.capacity_in_bytes(), n);
            }

            {
                request req(
                    "POST / HTTP/1.1\r\n"
                    "\r\n");
                auto const n =
                    req.capacity_in_bytes();
                BOOST_TEST_EQ(
                    req.buffer(),
                    "POST / HTTP/1.1\r\n\r\n");
                BOOST_TEST_EQ(
                    req.capacity_in_bytes(), n);
            }

            {
                BOOST_TEST_THROWS(
                    request(
                        "POST / HTTP/1.1"
                        "\r\n"),
                    std::invalid_argument
                );
            }

            {
                request req(
                    "POST / HTTP/1.1\r\n"
                    "User-Agent: test\r\n"
                    "Server: test\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n");
                auto const n =
                    req.capacity_in_bytes();
                BOOST_TEST_GT(n, 0);
                req.clear();
                BOOST_TEST_EQ(
                    req.buffer(),
                    "GET / HTTP/1.1\r\n\r\n");
                BOOST_TEST_EQ(
                    req.capacity_in_bytes(), n);
            }

            {
                response res(
                    "HTTP/1.1 404 Not Found\r\n"
                    "\r\n");
                auto const n =
                    res.capacity_in_bytes();
                BOOST_TEST_EQ(
                    res.buffer(),
                    "HTTP/1.1 404 Not Found\r\n\r\n");
                BOOST_TEST_EQ(
                    res.capacity_in_bytes(), n);
            }

            {
                BOOST_TEST_THROWS(
                    response(
                        "HTTP/1.1 404 Not Found"
                        "\r\n"),
                    std::invalid_argument);
            }

            {
                response res(
                    "HTTP/1.1 404 Not Found\r\n"
                    "User-Agent: test\r\n"
                    "Server: test\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n");
                auto const n =
                    res.capacity_in_bytes();
                BOOST_TEST_GT(n, 0);
                res.clear();
                BOOST_TEST_EQ(
                    res.buffer(),
                    "HTTP/1.1 200 OK\r\n\r\n");
                BOOST_TEST_EQ(
                    res.capacity_in_bytes(), n);
            }
        }

        // reserve_bytes(std::size_t)
        {
            // default request
            {
                request req;
                BOOST_TEST_EQ(
                    req.capacity_in_bytes(), 0);

                // first allocation
                req.reserve_bytes(100);
                auto const n =
                    req.capacity_in_bytes();
                BOOST_TEST_GE(n, 100);
                BOOST_TEST_EQ(req.buffer(),
                    "GET / HTTP/1.1\r\n\r\n");

                // no reallocation
                req.reserve_bytes(n);
                BOOST_TEST_EQ(
                    req.capacity_in_bytes(), n);

                // no reallocation
                req.reserve_bytes(n / 2);
                BOOST_TEST_EQ(
                    req.capacity_in_bytes(), n);

                // reallocation
                req.reserve_bytes(n + 1);
                BOOST_TEST_GT(
                    req.capacity_in_bytes(), n);
                BOOST_TEST_EQ(req.buffer(),
                    "GET / HTTP/1.1\r\n\r\n");
            }

            // response
            {
                core::string_view const cs =
                    "HTTP/1.1 200 OK\r\n"
                    "Server: test\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n";
                response res(cs);
                auto n = res.capacity_in_bytes();
                BOOST_TEST_GT(n, 0);

                // no reallocation
                res.reserve_bytes(n);
                BOOST_TEST_EQ(
                    res.capacity_in_bytes(), n);
                BOOST_TEST_EQ(res.buffer(), cs);
            }
        }

        // shrink_to_fit()
        {
            // default fields
            {
                fields f;
                f.shrink_to_fit();
                BOOST_TEST_EQ(
                    f.buffer(),
                    "\r\n");
                BOOST_TEST_EQ(
                    f.capacity_in_bytes(), 0);
            }

            // default request
            {
                request req;
                req.shrink_to_fit();
                BOOST_TEST_EQ(
                    req.buffer(),
                    "GET / HTTP/1.1\r\n\r\n");
                BOOST_TEST_EQ(
                    req.capacity_in_bytes(), 0);
            }

            // default response
            {
                response res;
                res.shrink_to_fit();
                BOOST_TEST_EQ(
                    res.buffer(),
                    "HTTP/1.1 200 OK\r\n\r\n");
                BOOST_TEST_EQ(
                    res.capacity_in_bytes(), 0);
            }

            {
                core::string_view const cs =
                    "digest: ffce\r\n"
                    "type: 3\r\n"
                    "\r\n";
                fields f(cs);
                f.reserve_bytes(
                    f.capacity_in_bytes() * 2);
                auto const n =
                    f.capacity_in_bytes();
                f.shrink_to_fit();
                BOOST_TEST_LT(
                    f.capacity_in_bytes(), n);
                BOOST_TEST_EQ(f.buffer(), cs);
            }

            {
                core::string_view const cs =
                    "POST / HTTP/1.1\r\n"
                    "User-Agent: test\r\n"
                    "Server: test\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n";
                request req(cs);
                req.reserve_bytes(
                    req.capacity_in_bytes() * 2);
                auto const n =
                    req.capacity_in_bytes();
                req.shrink_to_fit();
                BOOST_TEST_LT(
                    req.capacity_in_bytes(), n);
                BOOST_TEST_EQ(req.buffer(), cs);
            }

            {
                core::string_view const cs =
                    "HTTP/1.1 404 Not Found\r\n"
                    "User-Agent: test\r\n"
                    "Server: test\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n";
                response res(cs);
                res.reserve_bytes(
                    res.capacity_in_bytes() * 2);
                auto const n =
                    res.capacity_in_bytes();
                res.shrink_to_fit();
                BOOST_TEST_LT(
                    res.capacity_in_bytes(), n);
                BOOST_TEST_EQ(res.buffer(), cs);
            }
        }
    }

    void
    testAppend()
    {
        // append(field, string_view)

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append(field::server, "y");
            },
            "Server: y\r\n"
            "\r\n");

        check(
            "Cookie: x\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.append(field::server, "y");
            },
            "Cookie: x\r\n"
            "Server: y\r\n"
            "\r\n");

        check(
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_THROWS(
                    f.append(field::server, "bad\r\nvalue"),
                    system::system_error);
            },
            "\r\n");

        // append(string_view, string_view)

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append("Server", "y");
            },
            "Server: y\r\n"
            "\r\n");

        check(
            "Cookie: x\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.append("Server", "y");
            },
            "Cookie: x\r\n"
            "Server: y\r\n"
            "\r\n");

        // append(string_view, string_view) rfc compliance
        //
        //    field-line     = field-name ":" OWS field-value OWS
        //
        //    field-name     = token
        //    token          = 1*tchar
        //    tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
        //                   / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
        //                   / DIGIT / ALPHA
        //                   ; any VCHAR, except delimiters
        //
        //    field-value    = *field-content
        //    field-content  = field-vchar
        //                      [ 1*( SP / HTAB / field-vchar ) field-vchar ]
        //    field-vchar    = VCHAR / obs-text
        //    obs-text       = %x80-FF

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append("!#$%&'*+-.^_`|~1A", "     A \t \x80\xffZ     ");
            },
            "!#$%&'*+-.^_`|~1A: A \t \x80\xffZ\r\n"
            "\r\n");

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append(
                    "!#$%&'*+-.^_`|~1A", "\r\n\t  \r\n   AB\r\n C  \r\n\t");
            },
            "!#$%&'*+-.^_`|~1A: AB   C\r\n"
            "\r\n");

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append("A", "A\r\n\tB\r\n C");
            },
            "A: A  \tB   C\r\n"
            "\r\n");

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append("A", "custom: rawr\r\n\tB\r\n C");
            },
            "A: custom: rawr  \tB   C\r\n"
            "\r\n");

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append("A", "   \t    \r\n  \r\n\t  \t       \t   \r\n ");
            },
            "A:\r\n"
            "\r\n");

        check_error(
            "\r\n",
            [](fields_base& f)
            {
                system::error_code ec;

                // ends with invalid obs-fold
                f.append("X", "AB\r\n C  \r\n", ec);
                BOOST_TEST(ec == error::bad_field_value);
                BOOST_TEST_THROWS(
                    f.append("X", "AB\r\n C  \r\n"),
                    system::system_error);

                // contains invalid obs-fold between {AB, C}
                ec = {};
                f.append("X", "\r\n\x09  \r\n   AB: rawr\r\nC", ec);
                BOOST_TEST(ec == error::bad_field_smuggle);

                ec = {};
                f.append("X", "         \r", ec);
                BOOST_TEST(ec == error::bad_field_value);

                // empty field name
                ec = {};
                f.append("", "ABC", ec);
                BOOST_TEST(ec == error::bad_field_name);

                std::vector<char const *> strs = {
                    "\r\nABC", "\rABC", "A\rBC",
                    "ABC\r",   "\nABC", "A\nBC",
                    "ABC\n",   "\r",    "\n"};

                for (auto const str : strs)
                {
                    ec = {};
                    f.append("X", str, ec);
                    BOOST_TEST(ec.failed());
                }
            });

        // empty value should not
        // have a prepended space
        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append("X", "");
            },
            "X:\r\n"
            "\r\n");

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.append("X", "");
                f.append("Y", "");
            },
            "X:\r\n"
            "Y:\r\n"
            "\r\n");
    }

    void
    testInsert()
    {
        // insert(iterator, field, string_view)

        check(
            "T: 1\r\n"
            "\r\n",
            [](fields_base& f)
            {
                {
                    system::error_code ec;
                    auto it = f.insert(f.find("T"), field::server, "x", ec);
                    BOOST_TEST(!ec.failed());
                    BOOST_TEST(it == f.find(field::server));
                }
                f.erase(field::server);
                {
                    auto it = f.insert(f.find("T"), field::server, "x");
                    BOOST_TEST(it == f.find(field::server));
                }
            },
            "Server: x\r\n"
            "T: 1\r\n"
            "\r\n");

        check(
            "T: 1\r\n"
            "U: 2\r\n"
            "\r\n",
            [](fields_base& f)
            {
                {
                    system::error_code ec;
                    auto pos = f.find("T");
                    auto it = f.insert(f.find("U"), field::server, "x", ec);
                    BOOST_TEST(!ec.failed());
                    BOOST_TEST(it == f.find(field::server));
                    BOOST_TEST(pos == f.find("T"));
                }
                f.erase(field::server);
                {
                    auto pos = f.find("T");
                    auto it = f.insert(f.find("U"), field::server, "x");
                    BOOST_TEST(it == f.find(field::server));
                    BOOST_TEST(pos == f.find("T"));
                }
            },
            "T: 1\r\n"
            "Server: x\r\n"
            "U: 2\r\n"
            "\r\n");

        check_error(
            "T: 1\r\n"
            "U: 2\r\n"
            "\r\n",
            [](fields_base& f)
            {
                system::error_code ec;
                f.insert(f.find("U"), field::server, "a\r\nb", ec);
                BOOST_TEST(ec.failed());
                BOOST_TEST_THROWS(
                    f.insert(f.find("U"), field::server, "a\r\nb"),
                    system::system_error);
            });

        // insert(iterator, string_view, string_view)

        check(
            "T: 1\r\n"
            "\r\n",
            [](fields_base& f)
            {
                auto it = f.insert(f.find("T"), "Server", "x");
                BOOST_TEST(it == f.find("Server"));
            },
            "Server: x\r\n"
            "T: 1\r\n"
            "\r\n");

        check(
            "T: 1\r\n"
            "U: 2\r\n"
            "\r\n",
            [](fields_base& f)
            {
                auto pos = f.find("T");
                auto it = f.insert(f.find("U"), "Server", "x");
                BOOST_TEST(it == f.find("Server"));
                BOOST_TEST(pos == f.find("T"));
            },
            "T: 1\r\n"
            "Server: x\r\n"
            "U: 2\r\n"
            "\r\n");

        check_error(
            "T: 1\r\n"
            "U: 2\r\n"
            "\r\n",
            [](fields_base& f)
            {
                system::error_code ec;

                f.insert(f.find("U"), "Ser ver", "x", ec);
                BOOST_TEST(ec == error::bad_field_name);
                BOOST_TEST_THROWS(
                    f.insert(f.find("U"), "Ser ver", "x"),
                    system::system_error);

                ec = {};
                f.insert(f.find("U"), " Server", "x", ec);
                BOOST_TEST(ec == error::bad_field_name);

                ec = {};
                f.insert(f.find("U"), "Server ", "x", ec);
                BOOST_TEST(ec == error::bad_field_name);
            });

        // self-intersect

        check(
            "Connection: close\r\n"
            "Set-Cookie: 0\r\n"
            "User-Agent: boost\r\n"
            "Set-Cookie: 1\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.insert(
                    f.begin(),
                    f.find(field::user_agent)->value,
                    f.find(field::connection)->value);
            },
            "boost: close\r\n"
            "Connection: close\r\n"
            "Set-Cookie: 0\r\n"
            "User-Agent: boost\r\n"
            "Set-Cookie: 1\r\n"
            "\r\n");

        check(
            "Connection: close\r\n"
            "Set-Cookie: 0\r\n"
            "User-Agent: boost\r\n"
            "Set-Cookie: 1\r\n"
            "\r\n",
            [](fields_base& f)
            {
                // reserve
                f.reserve_bytes(f.capacity_in_bytes() * 2);
                f.insert(
                    f.begin(),
                    f.find(field::user_agent)->value,
                    f.find(field::connection)->value);
            },
            "boost: close\r\n"
            "Connection: close\r\n"
            "Set-Cookie: 0\r\n"
            "User-Agent: boost\r\n"
            "Set-Cookie: 1\r\n"
            "\r\n");
    }

    void
    testErase()
    {
        // erase(iterator)

        check(
            "Server: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.erase(f.find("Server"));
            },
            "\r\n");

        check(
            "Cookie: x\r\n"
            "Server: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.erase(f.find("Server"));
            },
            "Cookie: x\r\n"
            "\r\n");

        //
        // erase(field)
        //

        // no match
        check(
            "Server: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase(field::connection), 0U);
            },
            "Server: y\r\n"
            "\r\n");

        // one match
        check(
            "Server: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase(field::server), 1);
            },
            "\r\n");

        // different capitalization
        check(
            "server: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase(field::server), 1);
            },
            "\r\n");

        // three matches, different capitalization
        check(
            "Server: x\r\n"
            "server: y\r\n"
            "SERVER: z\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase(field::server), 3);
            },
            "\r\n");

        // three matches, three unmatched
        check(
            "T: 1\r\n"
            "Server: x\r\n"
            "U: 2\r\n"
            "Server: y\r\n"
            "Server: z\r\n"
            "V: 3\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase(field::server), 3);
            },
            "T: 1\r\n"
            "U: 2\r\n"
            "V: 3\r\n"
            "\r\n");

        //
        // erase(string_view)
        //

        // one match, different case
        check(
            "Server: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase("server"), 1);
            },
            "\r\n");

        // one match, different case
        check(
            "server: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase("Server"), 1);
            },
            "\r\n");

        // three matches
        check(
            "T: 1\r\n"
            "Server: x\r\n"
            "U: 2\r\n"
            "Server: y\r\n"
            "Server: z\r\n"
            "V: 3\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase("Server"), 3);
            },
            "T: 1\r\n"
            "U: 2\r\n"
            "V: 3\r\n"
            "\r\n");

        // three matches, unknown id
        check(
            "T: 1\r\n"
            "Server: Boost\r\n"
            "T: 2\r\n"
            "Connection: close\r\n"
            "T: 3\r\n"
            "U: 4\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase("T"), 3);
            },
            "Server: Boost\r\n"
            "Connection: close\r\n"
            "U: 4\r\n"
            "\r\n");

        // no matches
        check(
            "Connection: keep-alive\r\n"
            "Server: Boost\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase("Accept"), 0U);
            },
            "Connection: keep-alive\r\n"
            "Server: Boost\r\n"
            "\r\n");

        // unknown field name
        check(
            "Connection: keep-alive\r\n"
            "X: 1\r\n"
            "Server: Boost\r\n"
            "X: 2\r\n"
            "Y: 3\r\n"
            "\r\n",
            [](fields_base& f)
            {
                BOOST_TEST_EQ(
                    f.erase("X"), 2);
            },
            "Connection: keep-alive\r\n"
            "Server: Boost\r\n"
            "Y: 3\r\n"
            "\r\n");
    }

    void
    testSet()
    {
        // set(iterator, string_view)

        check(
            "T: 1\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(f.find("T"), "2");
                system::error_code ec;
                f.set(f.find("T"), "2", ec);
                BOOST_TEST(!ec.failed());
            },
            "T: 2\r\n"
            "\r\n");

        check(
            "T: abc\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(f.find("T"), "2");
            },
            "T: 2\r\n"
            "\r\n");

        check(
            "T: 1\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(f.find("T"), "abcdefghijklmnopqrstuvwxyz");
            },
            "T: abcdefghijklmnopqrstuvwxyz\r\n"
            "\r\n");

        check(
            "T: 1\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(f.find("T"), "abcdefghijk\r\n lmnopqrst\r\n\tuvwxyz\r\n ");
            },
            "T: abcdefghijk   lmnopqrst  \tuvwxyz\r\n"
            "\r\n");

        check_error(
            "T: abc\r\n"
            "\r\n",
            [](fields_base& f)
            {
                system::error_code ec;
                f.set(f.find("T"), "\r\n", ec);
                BOOST_TEST_EQ(ec, error::bad_field_value);
                BOOST_TEST_THROWS(
                    f.set(f.find("T"), "\r\n"),
                    system::system_error);

                ec = {};
                f.set(
                    f.find("T"),
                    "abcdefghijk\r\nlmnopqrstuvwxyz",
                    ec);
                BOOST_TEST_EQ(ec, error::bad_field_smuggle);
                BOOST_TEST_THROWS(
                    f.set(
                        f.find("T"),
                        "abcdefghijk\r\nlmnopqrstuvwxyz"),
                    system::system_error);
            });

        // set(field, string_view)

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.set(field::server, "x");
                system::error_code ec;
                f.set(field::server, "x");
                BOOST_TEST(!ec.failed());
            },
            "Server: x\r\n"
            "\r\n");

        check(
            "Server: x\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(field::server, "y");
            },
            "Server: y\r\n"
            "\r\n");

        check(
            "T: 1\r\n"
            "Server: x\r\n"
            "T: 2\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(field::server, "y");
            },
            "T: 1\r\n"
            "T: 2\r\n"
            "Server: y\r\n"
            "\r\n");

        check(
            "Server: x1\r\n"
            "Server: x2\r\n"
            "Server: x3\r\n"
            "T: t\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(field::server, "y");
            },
            "T: t\r\n"
            "Server: y\r\n"
            "\r\n");

        check(
            "Server: x\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set(field::server, "\r\n x\r\n yz \r\n \r\n\t");
                system::error_code ec;
                f.set(field::server, "\r\n x\r\n yz \r\n \r\n\t", ec);
                BOOST_TEST(!ec.failed());
            },
            "Server: x   yz\r\n"
            "\r\n");

        check_error(
            "Server: x\r\n"
            "\r\n",
            [](fields_base& f)
            {
                system::error_code ec;
                f.set(field::server, "\r\n x\r\nyz \r\n \r\n\t", ec);
                BOOST_TEST_EQ(ec, error::bad_field_smuggle);
                BOOST_TEST_THROWS(
                    f.set(field::server, "\r\n x\r\nyz \r\n \r\n\t"),
                    system::system_error);

                ec = {};
                f.set(field::server, "yz\r\n\x01\x02\x03", ec);
                BOOST_TEST_EQ(ec, error::bad_field_smuggle);
                BOOST_TEST_THROWS(
                    f.set(field::server, "yz\r\n\x01\x02\x03"),
                    system::system_error);
            });

        // set(string_view, string_view)

        check(
            "\r\n",
            [](fields_base& f)
            {
                f.set("Server", "x");
            },
            "Server: x\r\n"
            "\r\n");

        check(
            "Server: x\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set("Server", "y");
            },
            "Server: y\r\n"
            "\r\n");

        check(
            "UnkownId0: w\r\n"
            "UnkownId1: x\r\n"
            "UnkownId2: y\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set("UnkownId1", "z");
            },
            "UnkownId0: w\r\n"
            "UnkownId2: y\r\n"
            "UnkownId1: z\r\n"
            "\r\n");

        check(
            "T: 1\r\n"
            "Server: xx\r\n"
            "T: 2\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set("Server", "y");
            },
            "T: 1\r\n"
            "T: 2\r\n"
            "Server: y\r\n"
            "\r\n");

        check(
            "Server: x1\r\n"
            "Server: x2\r\n"
            "Server: x3\r\n"
            "T: t\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set("Server", "y");
            },
            "T: t\r\n"
            "Server: y\r\n"
            "\r\n");

        check(
            "Connection: keep-alive\r\n"
            "Server: Boost\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set("Connection", "close");
            },
            "Server: Boost\r\n"
            "Connection: close\r\n"
            "\r\n");

        check(
            "Connection: keep-alive\r\n"
            "Server: Boost\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.set("Server", "\r\n hello    \r\n     world!!!!");
            },
            "Connection: keep-alive\r\n"
            "Server: hello           world!!!!\r\n"
            "\r\n");

        check_error(
            "\r\n",
            [](fields_base& f)
            {
                system::error_code ec;
                f.set(" invalid string", "valid string", ec);
                BOOST_TEST_EQ(ec, error::bad_field_name);
                BOOST_TEST_THROWS(
                    f.set(" invalid string", "valid string"),
                    system::system_error);

                ec = {};
                f.set("invalid\r\n string", "valid string", ec);
                BOOST_TEST_EQ(ec, error::bad_field_name);
                BOOST_TEST_THROWS(
                    f.set("invalid\r\n string", "valid string"),
                    system::system_error);

                ec = {};
                f.set("valid", "\r\ninvalid string", ec);
                BOOST_TEST_EQ(ec, error::bad_field_smuggle);
                BOOST_TEST_THROWS(
                    f.set("valid", "\r\ninvalid string"),
                    system::system_error);

                ec = {};
                f.set("valid", "invalid\x01\x02\r\nstring", ec);
                BOOST_TEST_EQ(ec, error::bad_field_value);
                BOOST_TEST_THROWS(
                    f.set("valid", "\r\ninvalid string"),
                    system::system_error);
            });
    }

    void
    testExpect()
    {
        // parse request
        {
            auto const check =
            []( metadata::expect_t md,
                core::string_view s)
            {
                request const req(s);
                BOOST_TEST_EQ(
                    req.metadata().expect.ec,
                    md.ec);
                BOOST_TEST_EQ(
                    req.metadata().expect.count,
                    md.count);
                BOOST_TEST_EQ(
                    req.metadata().expect.is_100_continue,
                    md.is_100_continue);
            };

            check(
                { {}, 0, false},
                "POST / HTTP/1.1\r\n"
                "\r\n");

            check(
                { {}, 1, true },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                { error::bad_expect, 1, false },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continueish\r\n"
                "\r\n");

            check(
                { error::bad_expect, 2, false },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                { error::bad_expect, 2, false },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Expect: 404-not-found\r\n"
                "\r\n");
        }

        // parse response
        {
            auto const check =
            [](core::string_view s)
            {
                response const res(s);
                BOOST_TEST_EQ(
                    res.metadata().expect.ec,
                    system::error_code());
                BOOST_TEST_EQ(
                    res.metadata().expect.count,
                    res.count(field::expect));
                BOOST_TEST_EQ(
                    res.metadata().expect.is_100_continue,
                    false);
            };

            check(
                "HTTP/1.1 200 OK\r\n"
                "\r\n");

            check(
                "HTTP/1.1 200 OK\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                "HTTP/1.1 200 OK\r\n"
                "Expect: 100-continueish\r\n"
                "\r\n");

            check(
                "HTTP/1.1 200 OK\r\n"
                "Expect: 100-continue\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                "HTTP/1.1 200 OK\r\n"
                "Expect: 100-continue\r\n"
                "Expect: 404-not-found\r\n"
                "\r\n");
        }

        // erase in response
        {
            response res(
                "HTTP/1.1 200 OK\r\n"
                "Expect: 100-continueish\r\n"
                "\r\n");
            auto it = res.find(field::expect);
            res.erase(it);
            BOOST_TEST(
                ! res.metadata().expect.ec.failed());
            BOOST_TEST_EQ(
                res.metadata().expect.count, 0);
            BOOST_TEST_EQ(
                res.metadata().expect.is_100_continue,
                false);
        }

        // erase, set in request
        {
            auto const check =
            []( metadata::expect_t md,
                void(*fn)(request&),
                core::string_view s)
            {
                request req(s);
                fn(req);
                BOOST_TEST_EQ(
                    req.metadata().expect.ec,
                    md.ec);
                BOOST_TEST_EQ(
                    req.metadata().expect.count,
                    md.count);
                BOOST_TEST_EQ(
                    req.metadata().expect.is_100_continue,
                    md.is_100_continue);
            };

            // erase

            check(
                { {}, 0, false },
                [](request& req)
                {
                    // erase one
                    auto it = req.find(
                        field::expect);
                    req.erase(it);
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                { {}, 0, false },
                [](request& req)
                {
                    // erase all
                    BOOST_TEST_EQ(req.erase(
                        field::expect), 1);
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                { {}, 0, false },
                [](request& req)
                {
                    BOOST_TEST_EQ(req.erase(
                        field::expect), 1);
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continueish\r\n"
                "\r\n");

            check(
                { {}, 1, true },
                [](request& req)
                {
                    auto it = req.find(field::expect);
                    BOOST_TEST_NE(it, req.end());
                    req.erase(it);
                    BOOST_TEST_EQ(
                        req.count(field::expect), 1);
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                { error::bad_expect, 1, false },
                [](request& req)
                {
                    auto it = req.find(field::expect);
                    BOOST_TEST_NE(it, req.end());
                    req.erase(it);
                    BOOST_TEST_EQ(
                        req.count(field::expect), 1);
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "Expect: 404-not-found\r\n"
                "\r\n");

            check(
                { {}, 1, true },
                [](request& req)
                {
                    auto it = req.find_last(
                        req.end(), field::expect);
                    BOOST_TEST_NE(it, req.end());
                    req.erase(it);
                    BOOST_TEST_EQ(
                        req.count(field::expect), 1);
                },
                "POST / HTTP/1.1\r\n"
                "Content-Length: 1234\r\n"
                "Expect: 100-continue\r\n"
                "Connection: close\r\n"
                "Expect: 404-not-found\r\n"
                "\r\n");

            // set

            check(
                { error::bad_expect, 1, false },
                [](request& req)
                {
                    req.set(
                        field::expect,
                        "100-continueish");
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continue\r\n"
                "\r\n");

            check(
                { {}, 1, true },
                [](request& req)
                {
                    req.set(
                        field::expect,
                        "100-continue");
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 100-continueish\r\n"
                "\r\n");

            check(
                { {}, 1, true },
                [](request& req)
                {
                    req.set(
                        field::expect,
                        "100-continue");
                },
                "POST / HTTP/1.1\r\n"
                "Expect: 500-server-error\r\n"
                "Expect: 404-not-found\r\n"
                "\r\n");
        }
    }

    void
    testIterators()
    {
        fields_base const& f = fields(
            "x: 1\r\n"
            "Accept: 2\r\n"
            "z: 3\r\n"
            "\r\n");

        // iterator
        // begin()
        // end()
        {
            BOOST_STATIC_ASSERT(std::is_same<
                fields_base::iterator,
                fields_base::const_iterator>::value);

            BOOST_TEST(
                fields_base::iterator() ==
                fields_base::iterator());

            fields_base::iterator it = f.begin();
            BOOST_TEST(it == f.begin());
            BOOST_TEST(it != f.end());

            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "x");
            BOOST_TEST_EQ(it->value, "1");

            ++it;
            BOOST_TEST_EQ(it->id, field::accept);
            BOOST_TEST_EQ(it->name, "Accept");
            BOOST_TEST_EQ(it->value, "2");

            {
                auto it0 = it++; // post-increment
                BOOST_TEST_EQ(it0->id, field::accept);
                BOOST_TEST_EQ(it0->name, "Accept");
                BOOST_TEST_EQ(it0->value, "2");
            }
            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "z");
            BOOST_TEST_EQ(it->value, "3");

            ++it;
            BOOST_TEST_EQ(it, f.end());

            --it;
            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "z");
            BOOST_TEST_EQ(it->value, "3");

            {
                auto it1 = it--; // post-decrement
                BOOST_TEST(!it1->id.has_value());
                BOOST_TEST_EQ(it1->name, "z");
                BOOST_TEST_EQ(it1->value, "3");
            }
            BOOST_TEST_EQ(it->id, field::accept);
            BOOST_TEST_EQ(it->name, "Accept");
            BOOST_TEST_EQ(it->value, "2");

            --it;
            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "x");
            BOOST_TEST_EQ(it->value, "1");

            BOOST_TEST_EQ(it, f.begin());
        }

        // reverse_iterator
        // rbegin()
        // rend()
        {
            BOOST_STATIC_ASSERT(std::is_same<
                fields_base::reverse_iterator,
                fields_base::const_reverse_iterator>::value);

            BOOST_TEST(
                fields_base::reverse_iterator() ==
                fields_base::reverse_iterator());

            fields_base::reverse_iterator it = f.rbegin();
            BOOST_TEST(it == f.rbegin());
            BOOST_TEST(it != f.rend());

            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "z");
            BOOST_TEST_EQ(it->value, "3");

            ++it;
            BOOST_TEST_EQ(it->id, field::accept);
            BOOST_TEST_EQ(it->name, "Accept");
            BOOST_TEST_EQ(it->value, "2");

            {
                auto it0 = it++; // post-increment
                BOOST_TEST_EQ(it0->id, field::accept);
                BOOST_TEST_EQ(it0->name, "Accept");
                BOOST_TEST_EQ(it0->value, "2");
            }
            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "x");
            BOOST_TEST_EQ(it->value, "1");

            ++it;
            BOOST_TEST_EQ(it, f.rend());

            --it;
            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "x");
            BOOST_TEST_EQ(it->value, "1");

            {
                auto it0 = it--; // post-decrement
                BOOST_TEST(!it0->id.has_value());
                BOOST_TEST_EQ(it0->name, "x");
                BOOST_TEST_EQ(it0->value, "1");
            }
            BOOST_TEST_EQ(it->id, field::accept);
            BOOST_TEST_EQ(it->name, "Accept");
            BOOST_TEST_EQ(it->value, "2");

            --it;
            BOOST_TEST(!it->id.has_value());
            BOOST_TEST_EQ(it->name, "z");
            BOOST_TEST_EQ(it->value, "3");

            BOOST_TEST_EQ(it, f.rbegin());
        }
    }

    void
    testObservers()
    {
        fields_base const& f =
            fields(
                "Content-Length: 42\r\n"
                "x: 1\r\n"
                "y: 2\r\n"
                "Set-Cookie: a\r\n"
                "x: 3\r\n"
                "z: 4\r\n"
                "Set-Cookie: b\r\n"
                "x: 5\r\n"
                "p: 6\r\n"
                "User-Agent: boost\r\n"
                "\r\n");

        // size()

        BOOST_TEST(f.size() == 10);

        // at(field)
        // at(string_view)

        BOOST_TEST(f.at("x") == "1");
        BOOST_TEST(f.at(field::set_cookie) == "a");
        BOOST_TEST_THROWS(
            f.at("accept"),
            std::out_of_range);
        BOOST_TEST_THROWS(
            f.at(field::accept),
            std::out_of_range);

        // exists(field)
        // exists(string_view)

        BOOST_TEST(f.exists("x"));
        BOOST_TEST(f.exists("X"));
        BOOST_TEST(f.exists("user-agent"));
        BOOST_TEST(! f.exists("a"));
        BOOST_TEST(f.exists(field::user_agent));
        BOOST_TEST(! f.exists(field::range));

        // count(field)
        // count(string_view)

        BOOST_TEST(f.count("x") == 3);
        BOOST_TEST(f.count("y") == 1);
        BOOST_TEST(f.count("q") == 0);
        BOOST_TEST(f.count(field::range) == 0);
        BOOST_TEST(f.count(field::user_agent) == 1);

        // find(field)
        // find(string_view)
        {
            BOOST_TEST(f.find("x")->name == "x");
            BOOST_TEST(f.find("accept") == f.end());
            BOOST_TEST(f.find(field::range) == f.end());
            BOOST_TEST(f.find(field::user_agent)->value == "boost");
        }

        // find(iterator, field)
        // find(iterator, string_view)
        {
            auto const it = f.find("y");
            BOOST_TEST(it != f.end());
            BOOST_TEST(it->value == "2");

            BOOST_TEST(f.find(it, "x")->value == "3");
            BOOST_TEST(f.find(it, "q") == f.end());
            BOOST_TEST(f.find(it, field::set_cookie)->value == "a");
            BOOST_TEST(f.find(it, field::range) == f.end());
        }

        // find_last(iterator, field)
        // find_last(iterator, string_view)
        {
            auto const it = f.find_last(f.end(), "x");
            BOOST_TEST(it != f.end());
            BOOST_TEST(it->value == "5");

            BOOST_TEST(f.find_last(it, "x")->value == "3");
            BOOST_TEST(f.find_last(it, "q") == f.end());
            BOOST_TEST(f.find_last(it, field::set_cookie)->value == "b");
            BOOST_TEST(f.find_last(it, field::range) == f.end());
        }

        // value_or(field, string_view)
        // value_or(string_view, string_view)
        {
            BOOST_TEST(f.value_or(
                field::set_cookie, "") == "a");
            BOOST_TEST(f.value_or(
                field::set_cookie2, "Q") == "Q");

            BOOST_TEST(f.value_or(
                "set-cookie", "") == "a");
            BOOST_TEST(f.value_or(
                "set-cookie2", "Q") == "Q");
        }
    }

    void
    testStream()
    {
        // operator<<
        {
            std::stringstream ss;
            response r;
            r.set(field::content_length, "42");
            r.set(field::connection, "Close");
            ss << r;
            BOOST_TEST_EQ(
                ss.str(),
                "HTTP/1.1 200 OK\n"
                "Content-Length: 42\n"
                "Connection: Close\n");
        }
    }

    void
    testSubrange()
    {
    }

    void
    run()
    {
        testCapacity();
        testAppend();
        testInsert();
        testErase();
        testSet();
        testExpect();
        testIterators();
        testObservers();
        testStream();
        testSubrange();
    }
};

TEST_SUITE(
    fields_base_test,
    "boost.http.fields_base");

} // http
} // boost
