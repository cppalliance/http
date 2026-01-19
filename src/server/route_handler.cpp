//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/route_handler.hpp>
#include <boost/http/string_body.hpp>
#include <boost/capy/ex/run_sync.hpp>

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

route_params&
route_params::
set_body(std::string s)
{
    if(! res.exists(http::field::content_type))
    {
        // VFALCO TODO detect Content-Type
        res.set(http::field::content_type,
            "text/plain; charset=UTF-8");
    }

    if(! res.exists(field::content_length))
    {
        res.set_payload_size(s.size());
    }

    serializer.start(res,
        http::string_body(std::string(s)));

    return *this;
}

} // http
} // boost
