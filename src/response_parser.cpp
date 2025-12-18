//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/response_parser.hpp>

namespace boost {
namespace http_proto {

response_parser::
response_parser(
    capy::polystore& ctx)
    : parser(
        ctx,
        detail::kind::response)
{
}

response
response_parser::
get() const
{
    return { response::view_tag, safe_get_header() };
}

} // http_proto
} // boost
