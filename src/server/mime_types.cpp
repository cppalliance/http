//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/mime_types.hpp>
#include <boost/http/server/mime_db.hpp>
#include <algorithm>
#include <cctype>

namespace boost {
namespace http {
namespace mime_types {

namespace {

struct ext_entry
{
    core::string_view ext;
    core::string_view type;
};

// Sorted by extension for binary search
constexpr ext_entry ext_db[] = {
    { "aac", "audio/aac" },
    { "avif", "image/avif" },
    { "bmp", "image/bmp" },
    { "bz", "application/x-bzip" },
    { "bz2", "application/x-bzip2" },
    { "cjs", "application/javascript" },
    { "css", "text/css" },
    { "csv", "text/csv" },
    { "flac", "audio/flac" },
    { "gif", "image/gif" },
    { "gz", "application/gzip" },
    { "htm", "text/html" },
    { "html", "text/html" },
    { "ico", "image/x-icon" },
    { "ics", "text/calendar" },
    { "jpeg", "image/jpeg" },
    { "jpg", "image/jpeg" },
    { "js", "text/javascript" },
    { "json", "application/json" },
    { "m4a", "audio/mp4" },
    { "m4v", "video/mp4" },
    { "manifest", "text/cache-manifest" },
    { "md", "text/markdown" },
    { "mjs", "text/javascript" },
    { "mp3", "audio/mpeg" },
    { "mp4", "video/mp4" },
    { "mpeg", "video/mpeg" },
    { "mpg", "video/mpeg" },
    { "oga", "audio/ogg" },
    { "ogg", "audio/ogg" },
    { "ogv", "video/ogg" },
    { "otf", "font/otf" },
    { "pdf", "application/pdf" },
    { "png", "image/png" },
    { "rtf", "application/rtf" },
    { "svg", "image/svg+xml" },
    { "tar", "application/x-tar" },
    { "tif", "image/tiff" },
    { "tiff", "image/tiff" },
    { "ttf", "font/ttf" },
    { "txt", "text/plain" },
    { "wasm", "application/wasm" },
    { "wav", "audio/wav" },
    { "weba", "audio/webm" },
    { "webm", "video/webm" },
    { "webp", "image/webp" },
    { "woff", "font/woff" },
    { "woff2", "font/woff2" },
    { "xhtml", "application/xhtml+xml" },
    { "xml", "application/xml" },
    { "zip", "application/zip" },
    { "7z", "application/x-7z-compressed" },
};

constexpr std::size_t ext_db_size = sizeof( ext_db ) / sizeof( ext_db[0] );

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

// Extract extension from path
core::string_view
get_extension( core::string_view path ) noexcept
{
    // Find last dot
    auto const pos = path.rfind( '.' );
    if( pos == core::string_view::npos )
        return path; // Assume it's just an extension
    return path.substr( pos + 1 );
}

// Binary search for extension
core::string_view
lookup_ext( core::string_view ext ) noexcept
{
    std::size_t lo = 0;
    std::size_t hi = ext_db_size;
    while( lo < hi )
    {
        auto const mid = lo + ( hi - lo ) / 2;
        auto const cmp = compare_icase( ext_db[mid].ext, ext );
        if( cmp < 0 )
            lo = mid + 1;
        else if( cmp > 0 )
            hi = mid;
        else
            return ext_db[mid].type;
    }
    return {};
}

} // (anon)

core::string_view
lookup( core::string_view path_or_ext ) noexcept
{
    if( path_or_ext.empty() )
        return {};

    // Skip leading dot if present
    if( path_or_ext[0] == '.' )
        path_or_ext.remove_prefix( 1 );

    auto const ext = get_extension( path_or_ext );
    return lookup_ext( ext );
}

core::string_view
extension( core::string_view type ) noexcept
{
    // Linear search for type -> extension
    // Could optimize with reverse map if needed
    for( std::size_t i = 0; i < ext_db_size; ++i )
    {
        if( compare_icase( ext_db[i].type, type ) == 0 )
            return ext_db[i].ext;
    }
    return {};
}

core::string_view
charset( core::string_view type ) noexcept
{
    auto const* entry = mime_db::lookup( type );
    if( entry )
        return entry->charset;
    return {};
}

std::string
content_type( core::string_view type_or_ext )
{
    core::string_view type;

    // Check if it looks like an extension
    if( ! type_or_ext.empty() &&
        ( type_or_ext[0] == '.' ||
          type_or_ext.find( '/' ) == core::string_view::npos ) )
    {
        type = lookup( type_or_ext );
        if( type.empty() )
            return {};
    }
    else
    {
        type = type_or_ext;
    }

    auto const cs = charset( type );
    if( cs.empty() )
        return std::string( type );

    std::string result;
    result.reserve( type.size() + 10 + cs.size() );
    result.append( type.data(), type.size() );
    result.append( "; charset=" );
    result.append( cs.data(), cs.size() );
    return result;
}

} // mime_types
} // http
} // boost
