//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/fresh.hpp>
#include <boost/http/field.hpp>

namespace boost {
namespace http {

namespace {

// Check if ETag matches If-None-Match
// Returns true if they match (response is fresh)
bool
etag_matches(
    core::string_view if_none_match,
    core::string_view etag ) noexcept
{
    if( if_none_match.empty() || etag.empty() )
        return false;

    // "*" matches any ETag
    if( if_none_match == "*" )
        return true;

    // Simple comparison - check if ETag appears in the list
    // In full implementation, would need to handle weak vs strong
    // and parse comma-separated list properly

    // Remove W/ prefix for comparison if present
    auto strip_weak = []( core::string_view s ) -> core::string_view
    {
        if( s.size() >= 2 &&
            ( s[0] == 'W' || s[0] == 'w' ) &&
            s[1] == '/' )
            return s.substr( 2 );
        return s;
    };

    auto const etag_val = strip_weak( etag );

    // Simple contains check for the ETag value
    auto pos = if_none_match.find( etag_val );
    if( pos != core::string_view::npos )
        return true;

    // Also check without weak prefix in if_none_match
    auto const inm_stripped = strip_weak( if_none_match );
    if( inm_stripped == etag_val )
        return true;

    return false;
}

// Parse HTTP date and compare
// Returns true if response's Last-Modified <= request's If-Modified-Since
// For simplicity, doing string comparison (works for RFC 7231 dates)
bool
not_modified_since(
    core::string_view if_modified_since,
    core::string_view last_modified ) noexcept
{
    if( if_modified_since.empty() || last_modified.empty() )
        return false;

    // HTTP dates in RFC 7231 format are lexicographically comparable
    // when in the same format (preferred format)
    // For a robust implementation, would parse dates properly
    return last_modified <= if_modified_since;
}

} // (anon)

bool
is_fresh(
    request const& req,
    response const& res ) noexcept
{
    // Get conditional request headers
    auto const if_none_match = req.value_or(
        field::if_none_match, "" );
    auto const if_modified_since = req.value_or(
        field::if_modified_since, "" );

    // If no conditional headers, not fresh
    if( if_none_match.empty() && if_modified_since.empty() )
        return false;

    // Get response caching headers
    auto const etag = res.value_or( field::etag, "" );
    auto const last_modified = res.value_or(
        field::last_modified, "" );

    // Check ETag first (stronger validator)
    if( ! if_none_match.empty() )
    {
        if( ! etag.empty() && etag_matches( if_none_match, etag ) )
            return true;
        // If If-None-Match present but doesn't match, not fresh
        return false;
    }

    // Fall back to If-Modified-Since
    if( ! if_modified_since.empty() && ! last_modified.empty() )
    {
        return not_modified_since( if_modified_since, last_modified );
    }

    return false;
}

} // http
} // boost
