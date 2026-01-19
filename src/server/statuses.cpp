//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/statuses.hpp>

namespace boost {
namespace http {
namespace statuses {

bool
is_empty( unsigned code ) noexcept
{
    switch( code )
    {
    case 204: // No Content
    case 205: // Reset Content
    case 304: // Not Modified
        return true;
    default:
        return false;
    }
}

bool
is_redirect( unsigned code ) noexcept
{
    switch( code )
    {
    case 300: // Multiple Choices
    case 301: // Moved Permanently
    case 302: // Found
    case 303: // See Other
    case 305: // Use Proxy
    case 307: // Temporary Redirect
    case 308: // Permanent Redirect
        return true;
    default:
        return false;
    }
}

bool
is_retry( unsigned code ) noexcept
{
    switch( code )
    {
    case 502: // Bad Gateway
    case 503: // Service Unavailable
    case 504: // Gateway Timeout
        return true;
    default:
        return false;
    }
}

} // statuses
} // http
} // boost
