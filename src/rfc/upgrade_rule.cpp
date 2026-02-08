//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/rfc/upgrade_rule.hpp>
#include <boost/http/rfc/token_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>

namespace boost {
namespace http {
namespace implementation_defined {

auto
upgrade_protocol_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    value_type t;
    // token
    {
        auto rv = grammar::parse(
            it, end, token_rule);
        if(! rv)
            return rv.error();
        t.name = *rv;
    }
    // [ "/" token ]
    if( it == end ||
        *it != '/')
        return t;
    ++it;
    auto rv = grammar::parse(
        it, end, token_rule);
    if(! rv)
        return rv.error();
    t.version = *rv;
    return t;
}

} // implementation_defined
} // http
} // boost
