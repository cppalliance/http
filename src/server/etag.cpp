//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/etag.hpp>
#include <cstdio>

namespace boost {
namespace http {

namespace {

// Simple FNV-1a hash for content
std::uint64_t
fnv1a_hash( core::string_view data ) noexcept
{
    constexpr std::uint64_t basis = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;

    std::uint64_t hash = basis;
    for( unsigned char c : data )
    {
        hash ^= c;
        hash *= prime;
    }
    return hash;
}

// Convert to null-terminated hex string
void
to_hex( std::uint64_t value, char* out ) noexcept
{
    constexpr char hex[] = "0123456789abcdef";
    for( int i = 15; i >= 0; --i )
    {
        out[i] = hex[value & 0xF];
        value >>= 4;
    }
    out[16] = '\0';
}

} // (anon)

std::string
etag( core::string_view body, etag_options opts )
{
    auto const hash = fnv1a_hash( body );

    char hex[17];
    to_hex( hash, hex );

    char buf[64];
    int n;
    if( opts.weak )
        n = std::snprintf( buf, sizeof(buf),
            "W/\"%zx-%s\"", body.size(), hex );
    else
        n = std::snprintf( buf, sizeof(buf),
            "\"%zx-%s\"", body.size(), hex );

    return std::string( buf, static_cast<std::size_t>(n) );
}

std::string
etag(
    std::uint64_t size,
    std::uint64_t mtime,
    etag_options opts )
{
    char buf[64];
    int n;
    if( opts.weak )
        n = std::snprintf( buf, sizeof(buf),
            "W/\"%llx-%llx\"",
            static_cast<unsigned long long>( size ),
            static_cast<unsigned long long>( mtime ) );
    else
        n = std::snprintf( buf, sizeof(buf),
            "\"%llx-%llx\"",
            static_cast<unsigned long long>( size ),
            static_cast<unsigned long long>( mtime ) );

    return std::string( buf, static_cast<std::size_t>(n) );
}

} // http
} // boost
