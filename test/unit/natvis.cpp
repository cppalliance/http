//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/fields.hpp>
#include <boost/http/request.hpp>
#include <boost/http/response.hpp>

#include "test_suite.hpp"

#include <boost/core/ignore_unused.hpp>

namespace boost {
namespace http {

struct natvis_test
{
    void
    run()
    {
        {
            request req(
                "GET / HTTP/1.1\r\n"
                "Connection: keep-alive\r\n"
                "Server: localhost\r\n"
                "\r\n");
            ignore_unused(req);
        }
        {
            std::error_condition ec(std::errc::address_in_use);
            response res(
                "HTTP/1.1 200 OK\r\n"
                "Connection: keep-alive\r\n"
                "Server: localhost\r\n"
                "\r\n");
            ignore_unused(res);
        }
    }
};

TEST_SUITE(natvis_test, "boost.http.natvis");

} // http
} // boost
