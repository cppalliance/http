//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/request_parser.hpp>

namespace boost {
namespace http_proto {

request_parser::
request_parser(
    rts::polystore& ctx)
    : parser(
        ctx,
        detail::kind::request)
{
}

static_request const&
request_parser::
get() const
{
    return safe_get_request();
}

} // http_proto
} // boost
