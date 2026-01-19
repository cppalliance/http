//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/range_parser.hpp>
#include <algorithm>
#include <cctype>
#include <charconv>

namespace boost {
namespace http {

namespace {

// Skip whitespace
void
skip_ws( core::string_view& s ) noexcept
{
    while( ! s.empty() && std::isspace(
        static_cast<unsigned char>( s.front() ) ) )
        s.remove_prefix( 1 );
}

// Parse integer
bool
parse_int( core::string_view& s, std::int64_t& out ) noexcept
{
    skip_ws( s );
    if( s.empty() )
        return false;

    auto const* begin = s.data();
    auto const* end = s.data() + s.size();
    auto [ptr, ec] = std::from_chars( begin, end, out );
    if( ec != std::errc() || ptr == begin )
        return false;

    s.remove_prefix( static_cast<std::size_t>( ptr - begin ) );
    return true;
}

// Parse a single range spec: "start-end" or "-suffix" or "start-"
bool
parse_range_spec(
    core::string_view& s,
    std::int64_t size,
    byte_range& out )
{
    skip_ws( s );
    if( s.empty() )
        return false;

    std::int64_t start = -1;
    std::int64_t end = -1;

    // Check for suffix range: "-suffix"
    if( s.front() == '-' )
    {
        s.remove_prefix( 1 );
        std::int64_t suffix;
        if( ! parse_int( s, suffix ) || suffix < 0 )
            return false;

        // Last 'suffix' bytes
        if( suffix == 0 )
            return false;

        if( suffix > size )
            suffix = size;

        out.start = size - suffix;
        out.end = size - 1;
        return true;
    }

    // Parse start
    if( ! parse_int( s, start ) || start < 0 )
        return false;

    skip_ws( s );
    if( s.empty() || s.front() != '-' )
        return false;

    s.remove_prefix( 1 ); // consume '-'

    // Check for "start-" (open-ended)
    skip_ws( s );
    if( s.empty() || s.front() == ',' )
    {
        // Open-ended: start to end of file
        out.start = start;
        out.end = size - 1;
        return start < size;
    }

    // Parse end
    if( ! parse_int( s, end ) || end < 0 )
        return false;

    // Validate
    if( start > end )
        return false;

    out.start = start;
    out.end = ( std::min )( end, size - 1 );

    return start < size;
}

} // (anon)

range_result
parse_range( std::int64_t size, core::string_view header )
{
    range_result result;
    result.type = range_result_type::malformed;

    if( size <= 0 )
    {
        result.type = range_result_type::unsatisfiable;
        return result;
    }

    // Must start with "bytes="
    skip_ws( header );
    if( header.size() < 6 )
        return result;

    // Case-insensitive "bytes=" check
    auto prefix = header.substr( 0, 6 );
    bool is_bytes = true;
    for( std::size_t i = 0; i < 5; ++i )
    {
        char c = static_cast<char>( std::tolower(
            static_cast<unsigned char>( prefix[i] ) ) );
        if( c != "bytes"[i] )
        {
            is_bytes = false;
            break;
        }
    }
    if( ! is_bytes || prefix[5] != '=' )
        return result;

    header.remove_prefix( 6 );

    // Parse range specs
    bool any_satisfiable = false;

    while( ! header.empty() )
    {
        skip_ws( header );
        if( header.empty() )
            break;

        byte_range range;
        if( parse_range_spec( header, size, range ) )
        {
            result.ranges.push_back( range );
            any_satisfiable = true;
        }

        skip_ws( header );
        if( ! header.empty() )
        {
            if( header.front() == ',' )
                header.remove_prefix( 1 );
            else
                break; // Invalid
        }
    }

    if( result.ranges.empty() )
    {
        result.type = range_result_type::unsatisfiable;
    }
    else if( any_satisfiable )
    {
        result.type = range_result_type::ok;
    }

    return result;
}

} // http
} // boost
