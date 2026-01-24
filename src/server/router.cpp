//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/router.hpp>

namespace boost {
namespace http {

route_params::
~route_params()
{
}

route_params&
route_params::
status(
    http::status code)
{
    res.set_start_line(code, res.version());
    return *this;
}

} // http
} // boost
