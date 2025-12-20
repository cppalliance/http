//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_SERVER_ROUTER_HPP
#define BOOST_HTTP_PROTO_SERVER_ROUTER_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/server/basic_router.hpp>

namespace boost {
namespace http_proto {

struct route_params;

/** A router for HTTP servers

    This is a specialization of `basic_router` using
    `route_params` as the handler parameter type.

    @see
        @ref basic_router,
        @ref route_params
*/
using router = basic_router<route_params>;

} // http_proto
} // boost

#endif