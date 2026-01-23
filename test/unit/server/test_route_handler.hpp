//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef TEST_ROUTE_HANDLER_HPP
#define TEST_ROUTE_HANDLER_HPP

#include <boost/http/server/route_handler.hpp>
#include <boost/http/server/router.hpp>
#include <string_view>

namespace boost {
namespace http {

struct test_route_params : route_params
{
    route_task send(std::string_view) override
    {
        co_return {};
    }

    route_task write_impl(capy::const_buffer_param) override
    {
        co_return {};
    }

    route_task end() override
    {
        co_return {};
    }
};

} // http
} // boost

#endif

