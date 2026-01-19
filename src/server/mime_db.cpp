//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/mime_db.hpp>
#include <algorithm>
#include <cctype>

namespace boost {
namespace http {
namespace mime_db {

namespace {

// Static database of common MIME types
// Sorted by type for binary search
constexpr mime_type_entry db[] = {
    { "application/gzip", "", false },
    { "application/javascript", "UTF-8", true },
    { "application/json", "UTF-8", true },
    { "application/octet-stream", "", false },
    { "application/pdf", "", false },
    { "application/rtf", "UTF-8", true },
    { "application/wasm", "", false },
    { "application/x-7z-compressed", "", false },
    { "application/x-bzip", "", false },
    { "application/x-bzip2", "", false },
    { "application/x-tar", "", false },
    { "application/xhtml+xml", "UTF-8", true },
    { "application/xml", "UTF-8", true },
    { "application/zip", "", false },
    { "audio/aac", "", false },
    { "audio/flac", "", false },
    { "audio/mp4", "", false },
    { "audio/mpeg", "", false },
    { "audio/ogg", "", false },
    { "audio/wav", "", false },
    { "audio/webm", "", false },
    { "font/otf", "", false },
    { "font/ttf", "", false },
    { "font/woff", "", false },
    { "font/woff2", "", false },
    { "image/avif", "", false },
    { "image/bmp", "", false },
    { "image/gif", "", false },
    { "image/jpeg", "", false },
    { "image/png", "", false },
    { "image/svg+xml", "UTF-8", true },
    { "image/tiff", "", false },
    { "image/webp", "", false },
    { "image/x-icon", "", false },
    { "text/cache-manifest", "UTF-8", true },
    { "text/calendar", "UTF-8", true },
    { "text/css", "UTF-8", true },
    { "text/csv", "UTF-8", true },
    { "text/html", "UTF-8", true },
    { "text/javascript", "UTF-8", true },
    { "text/markdown", "UTF-8", true },
    { "text/plain", "UTF-8", true },
    { "text/xml", "UTF-8", true },
    { "video/mp4", "", false },
    { "video/mpeg", "", false },
    { "video/ogg", "", false },
    { "video/webm", "", false },
};

constexpr std::size_t db_size = sizeof( db ) / sizeof( db[0] );

// Case-insensitive comparison
int
compare_icase( core::string_view a, core::string_view b ) noexcept
{
    auto const n = ( std::min )( a.size(), b.size() );
    for( std::size_t i = 0; i < n; ++i )
    {
        auto const ca = static_cast<unsigned char>(
            std::tolower( static_cast<unsigned char>( a[i] ) ) );
        auto const cb = static_cast<unsigned char>(
            std::tolower( static_cast<unsigned char>( b[i] ) ) );
        if( ca < cb )
            return -1;
        if( ca > cb )
            return 1;
    }
    if( a.size() < b.size() )
        return -1;
    if( a.size() > b.size() )
        return 1;
    return 0;
}

} // (anon)

mime_type_entry const*
lookup( core::string_view type ) noexcept
{
    // Binary search
    std::size_t lo = 0;
    std::size_t hi = db_size;
    while( lo < hi )
    {
        auto const mid = lo + ( hi - lo ) / 2;
        auto const cmp = compare_icase( db[mid].type, type );
        if( cmp < 0 )
            lo = mid + 1;
        else if( cmp > 0 )
            hi = mid;
        else
            return &db[mid];
    }
    return nullptr;
}

} // mime_db
} // http
} // boost
