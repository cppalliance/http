//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ROUTE_ABNF_HPP
#define BOOST_HTTP_SERVER_ROUTE_ABNF_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <boost/system/result.hpp>
#include <string>
#include <vector>

namespace boost {
namespace http {
namespace detail {

//------------------------------------------------

/** Type of route pattern token
*/
enum class route_token_type
{
    text,       // literal text
    param,      // :name parameter
    wildcard,   // *name wildcard
    group       // {...} optional group
};

//------------------------------------------------

/** A token in a parsed route pattern
*/
struct route_token
{
    route_token_type type;
    std::string value;              // text content or param name
    std::vector<route_token> children;  // group contents

    route_token() = default;

    route_token(
        route_token_type t,
        std::string v)
        : type(t)
        , value(std::move(v))
    {
    }
};

//------------------------------------------------

/** Result of parsing a route pattern
*/
struct route_pattern
{
    std::vector<route_token> tokens;
    std::string original;
};

//------------------------------------------------

/** Parse a route pattern string

    Parses a path-to-regexp style route pattern into tokens.

    @par Grammar
    @code
    path      = *token
    token     = text / param / wildcard / group
    text      = 1*(char / escaped-char)
    param     = ":" name
    wildcard  = "*" name
    group     = "{" *token "}"
    name      = identifier / quoted-name
    @endcode

    @param pattern The route pattern string to parse

    @return A result containing the parsed route pattern, or
    an error if parsing failed
*/
system::result<route_pattern>
parse_route_pattern(core::string_view pattern);

} // detail
} // http
} // boost

#endif
