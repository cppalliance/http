//
// Copyright (c) 2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/detail/config.hpp>
#include <boost/http/fields.hpp>
#include <boost/http/request.hpp>
#include <boost/http/response.hpp>

#include "test_suite.hpp"

#include <stdexcept>
#include <string>

// These ensure that limits is compiled correctly
#if \
    defined(BOOST_HTTP_DYN_LINK) || \
    ( defined(BOOST_ALL_DYN_LINK) && ! defined(BOOST_HTTP_STATIC_LINK) )
#error "Limits should not be built with shared linking."
#endif

#ifndef BOOST_HTTP_TEST_LIMITS
#error "Limits should be built with BOOST_HTTP_TEST_LIMITS."
#endif

namespace boost {
namespace http {

constexpr auto max_offset =
    std::numeric_limits<std::uint8_t>::max();

class limits_test
{
    static
    std::string
    make_str(std::size_t n)
    {
        return std::string(n, '*');
    }

public:
    void
    testLimits()
    {
        // request
        {
            request req;
            core::string_view const cs =
                "POST  HTTP/1.1\r\n"
                "\r\n";
            const auto remain = max_offset - cs.size();

            BOOST_TEST_THROWS(
                req.set_start_line(
                    method::post,
                    make_str(remain + 1),
                    version::http_1_1),
                std::length_error);

            req.set_start_line(
                method::post,
                make_str(remain),
                version::http_1_1);

            BOOST_TEST_EQ(
                req.buffer().size(), max_offset);
        }

        // response
        {
            response res;
            core::string_view const cs =
                "HTTP/1.1 200 \r\n"
                "\r\n";
            const auto remain = max_offset - cs.size();

            BOOST_TEST_THROWS(
                res.set_start_line(
                    200,
                    make_str(remain + 1),
                    version::http_1_1),
                std::length_error);

            res.set_start_line(
                200,
                make_str(remain),
                version::http_1_1);

            BOOST_TEST_EQ(
                res.buffer().size(), max_offset);
        }
    }

    void
    testFields()
    {
        core::string_view const cs =
            ": \r\n"
            "\r\n";
        const auto remain = max_offset - cs.size();

        // append
        {
            fields f;
            BOOST_TEST_THROWS(
                f.append(
                    make_str(remain / 2 + 1),
                    make_str(remain / 2 + 1)),
                std::length_error);

            f.append(
                make_str(remain / 2),
                make_str(remain / 2 + 1));

            BOOST_TEST_EQ(
                f.buffer().size(), max_offset);
        }

        // set
        {
            fields f;
            BOOST_TEST_THROWS(
                f.set(
                    make_str(remain / 2 + 1),
                    make_str(remain / 2 + 1)),
                std::length_error);

            f.set(
                make_str(remain / 2),
                make_str(remain / 2 + 1));

            BOOST_TEST_EQ(
                f.buffer().size(), max_offset);
        }
    }

    void
    run()
    {
        testLimits();
        testFields();
    }
};

TEST_SUITE(
    limits_test,
    "boost.http.limits");

} // http
} // boost
