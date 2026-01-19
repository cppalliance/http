//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_RANGE_PARSER_HPP
#define BOOST_HTTP_SERVER_RANGE_PARSER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <cstdint>
#include <vector>

namespace boost {
namespace http {

/** A single byte range.

    Represents an inclusive byte range within a resource.
    Both `start` and `end` are zero-based byte offsets.
*/
struct byte_range
{
    /// Start of range (inclusive).
    std::int64_t start = 0;

    /// End of range (inclusive).
    std::int64_t end = 0;
};

/** Result type for range header parsing.
*/
enum class range_result_type
{
    /// Ranges parsed successfully.
    ok,

    /// Range is not satisfiable (416 response).
    unsatisfiable,

    /// Range header is malformed (ignore it).
    malformed
};

/** Result of parsing a Range header.
*/
struct range_result
{
    /// The parsed ranges (empty if malformed or unsatisfiable).
    std::vector<byte_range> ranges;

    /// The result type.
    range_result_type type = range_result_type::ok;
};

/** Parse an HTTP Range header.

    Parses a Range header value (e.g. "bytes=0-499" or
    "bytes=0-499, 1000-1499") and returns the requested
    byte ranges adjusted to the resource size.

    @par Example
    @code
    // Single range
    auto result = parse_range( 10000, "bytes=0-499" );
    if( result.type == range_result_type::ok )
    {
        // result.ranges[0].start == 0
        // result.ranges[0].end == 499
    }

    // Suffix range (last 500 bytes)
    auto result2 = parse_range( 10000, "bytes=-500" );
    // result2.ranges[0].start == 9500
    // result2.ranges[0].end == 9999
    @endcode

    @param size The size of the resource in bytes.

    @param header The Range header value.

    @return A range_result containing the parsed ranges and
    result type.

    @see byte_range, range_result, range_result_type
*/
BOOST_HTTP_DECL
range_result
parse_range(std::int64_t size, core::string_view header);

} // http
} // boost

#endif
