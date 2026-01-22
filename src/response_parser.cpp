//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/response_parser.hpp>

namespace boost {
namespace http {

response_parser::
response_parser(
    http::polystore& ctx)
    : parser(
        ctx,
        detail::kind::response)
{
}

static_response const&
response_parser::
get() const
{
    return safe_get_response();
}

} // http
} // boost
