//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/rfc/content_disposition_rule.hpp>
#include <boost/http_proto/rfc/token_rule.hpp>
#include <boost/http_proto/rfc/parameter.hpp>

namespace boost {
namespace http_proto {
namespace implementation_defined {
auto
content_disposition_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept->
        system::result<value_type>
{   
    auto type = grammar::parse(it, end, token_rule);
    if(!type)
        return type.error();

    auto params = grammar::parse(it, end, parameters_rule);
    if(!params)
        return params.error();

    return value_type{ *type, *params };
}

} // implementation_defined
} // http_proto
} // boost
