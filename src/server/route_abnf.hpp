//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
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

//------------------------------------------------

/** Options for route matching
*/
struct match_options
{
    bool case_sensitive;  ///< Text comparison mode
    bool strict;          ///< Trailing slash matters
    bool end;             ///< true = full match, false = prefix match
};

//------------------------------------------------

/** Result of matching a path against a pattern
*/
struct match_params
{
    std::vector<std::pair<std::string, std::string>> params;  ///< Captured parameters
    std::size_t matched_length;  ///< Characters consumed from path
};

//------------------------------------------------

/** Match a decoded path against a route pattern

    Attempts to match the given path against the pattern,
    extracting any captured parameters.

    @param path The decoded path to match (not URL-encoded)
    @param pattern The parsed route pattern
    @param opts Matching options

    @return The captured parameters and match length if successful,
    or an error if the path doesn't match
*/
system::result<match_params>
match_route(
    core::string_view path,
    route_pattern const& pattern,
    match_options const& opts);

} // detail
} // http
} // boost

#endif
