//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2024 Christian Mazakas
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/fields.hpp>

#include <boost/http/field.hpp>
#include <boost/core/detail/string_view.hpp>

#include "test_helpers.hpp"
#include "test_suite.hpp"

#include <stdexcept>
#include <utility>

namespace boost {
namespace http {

struct fields_test
{
    void
    modify(
        core::string_view before,
        void (*pf)(fields&),
        core::string_view after)
    {
        fields f0(before);
        fields f1(after);
        fields f(f0);
        (*pf)(f);
        BOOST_TEST_EQ(f.buffer(),
            f1.buffer());
    }

    //--------------------------------------------

    void
    testSpecial()
    {
        core::string_view const cs1 =
            "Connection: close\r\n"
            "Set-Cookie: 0\r\n"
            "User-Agent: boost\r\n"
            "Set-Cookie: 1\r\n"
            "\r\n";

        core::string_view const cs2 =
            "x: 1\r\n"
            "y: 2\r\n"
            "z: 3\r\n"
            "\r\n";

        // ~fields()
        // fields()
        {
            fields f;
            BOOST_TEST_EQ(
                f.buffer(), "\r\n");
            BOOST_TEST_EQ(
                f.buffer().data(),
                fields().buffer().data());
        }

        // fields(fields&&)
        {
            {
                fields f1(cs1);
                fields f2(std::move(f1));
                test_fields(f1, "\r\n");
                test_fields(f2, cs1);
            }
            {
                fields f1;
                fields f2(std::move(f1));
                test_fields(f1, "\r\n");
                test_fields(f2, "\r\n");
                BOOST_TEST_EQ(
                    f1.buffer().data(),
                    f2.buffer().data());
            }
        }

        // fields(fields const&)
        {
            {
                fields f1(cs1);
                fields f2(f1);
                test_fields(f1, cs1);
                test_fields(f2, cs1);
                BOOST_TEST_NE(
                    f1.buffer().data(),
                    f2.buffer().data());
            }
            {
                fields f1;
                fields f2(f1);
                test_fields(f1, "\r\n");
                test_fields(f2, "\r\n");
                BOOST_TEST_EQ(
                    f1.buffer().data(),
                    f2.buffer().data());
            }
        }

        // operator=(fields&&)
        {
            {
                fields f1(cs1);
                fields f2;
                f2 = std::move(f1);
                test_fields(f1, "\r\n");
                test_fields(f2, cs1);
            }
            {
                fields f1(cs1);
                fields f2(cs2);
                f2 = std::move(f1);
                test_fields(f1, "\r\n");
                test_fields(f2, cs1);
            }
            {
                fields f1;
                fields f2(cs1);
                f2 = std::move(f1);
                test_fields(f1, "\r\n");
                test_fields(f2, "\r\n");
                BOOST_TEST_EQ(
                    f1.buffer().data(),
                    f2.buffer().data());
            }
        }

        // operator=(fields const&)
        {
            {
                fields f1(cs1);
                fields f2;
                f2 = f1;
                test_fields(f1, cs1);
                test_fields(f2, cs1);
                BOOST_TEST_NE(
                    f1.buffer().data(),
                    f2.buffer().data());
            }
            {
                fields f1(cs1);
                fields f2(
                    "x: 1\r\n"
                    "y: 2\r\n"
                    "z: 3\r\n"
                    "\r\n");
                f2 = f1;
                test_fields(f1, cs1);
                test_fields(f2, cs1);
                BOOST_TEST_NE(
                    f1.buffer().data(),
                    f2.buffer().data());
            }
            {
                fields f1;
                fields f2(cs1);
                f2 = f1;
                test_fields(f1, "\r\n");
                test_fields(f2, "\r\n");
                BOOST_TEST_EQ(
                    f1.buffer().data(),
                    f2.buffer().data());
            }
        }
    }

    void
    testObservers()
    {
        core::string_view const cs =
            "Connection: close\r\n"
            "Set-Cookie: 0\r\n"
            "User-Agent: boost\r\n"
            "Set-Cookie: 1\r\n"
            "\r\n";

        // fields_view_base::string()
        {
            fields f1(cs);
            BOOST_TEST_EQ(f1.buffer(), cs);
        }

        // fields_base::capacity_in_bytes()
        {
            {
                fields f;
                BOOST_TEST_EQ(
                    f.capacity_in_bytes(), 0U);
            }
            {
                fields f;
                f.reserve_bytes(100);
                BOOST_TEST_GE(
                    f.capacity_in_bytes(), 100U);
            }
            {
                fields f;
                f.reserve_bytes(100);
                f.shrink_to_fit();
                BOOST_TEST_GT(
                    f.capacity_in_bytes(), 0U);
            }
        }
    }

    void
    testInitialSize()
    {
        auto check_default =[](fields& f)
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
            fields& f,
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
            fields f;
            check_default(f);
        }

        {
            fields f(0);
            check_default(f);
        }

        {
            fields f(0, 0);
            BOOST_TEST_THROWS(
                f.append(field::host, "www.google.com"), std::length_error);
            BOOST_TEST_EQ(
                f.max_capacity_in_bytes(), 0);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = init;

            fields f(init, cap);
            check(f, init, cap);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = 8192;

            fields f(init, cap);
            check(f, init, cap);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = 8192;

            fields f(init, cap);
            fields f2(2 * init, 2 * cap);
            check(f, init, cap);

            f = f2;
            // check(f, init, 2 * cap);
            // check(f2, 2 * init, 2 * cap);
        }

        {
            std::size_t init = 4096;
            std::size_t cap = 8192;

            fields f(init, cap);
            fields f2(2 * init, 2 * cap);
            check(f, init, cap);

            f = std::move(f2);
            // check(f, 2 * init, 2 * cap);
        }

        {
            BOOST_TEST_THROWS(
                fields(1024, 0), std::logic_error);

            BOOST_TEST_THROWS(
                fields(1024, 512), std::logic_error);
        }
    }

    void
    run()
    {
        testSpecial();
        testObservers();
        testInitialSize();
    }
};

TEST_SUITE(
    fields_test,
    "boost.http.fields");

} // http
} // boost
