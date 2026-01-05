//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/rfc/parameter.hpp>
#include <boost/http_proto/rfc/token_rule.hpp>
#include <boost/http_proto/rfc/quoted_token_rule.hpp>
#include <boost/http_proto/rfc/detail/ws.hpp>

#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/optional_rule.hpp>
#include <boost/url/grammar/tuple_rule.hpp>
#include <boost/url/grammar/literal_rule.hpp>

namespace boost {
namespace http_proto {
namespace implementation_defined {
auto
parameter_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    auto name = grammar::parse(it, end, token_rule);
    if(!name)
        return name.error();

    if(it == end)
        BOOST_HTTP_PROTO_RETURN_EC(
            grammar::error::need_more);

    if(*it++ != '=')
        BOOST_HTTP_PROTO_RETURN_EC(
            grammar::error::mismatch);

    auto value = grammar::parse(
        it, end, quoted_token_rule);
    if(!value)
        return value.error();

    return value_type{ *name, *value };
}

auto
parameters_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    constexpr auto ows = grammar::squelch(
        grammar::optional_rule(
            grammar::token_rule(detail::ws)));

    return grammar::parse(
        it, end, grammar::range_rule(
            grammar::tuple_rule(
                ows,
                grammar::squelch(grammar::literal_rule(";")),
                ows,
                parameter_rule)));
}
} // implementation_defined
} // http_proto
} // boost
