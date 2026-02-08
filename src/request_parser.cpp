//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/request_parser.hpp>

#include <memory>

namespace boost {
namespace http {

request_parser::
request_parser(
    std::shared_ptr<parser_config_impl const> cfg)
    : parser(std::move(cfg), detail::kind::request)
{
}

static_request const&
request_parser::
get() const
{
    return safe_get_request();
}

} // http
} // boost
