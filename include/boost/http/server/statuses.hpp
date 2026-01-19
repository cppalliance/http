//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_STATUSES_HPP
#define BOOST_HTTP_SERVER_STATUSES_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/status.hpp>

namespace boost {
namespace http {

/** HTTP status code utilities.

    This namespace provides utility functions for working with
    HTTP status codes beyond the basic enum and string conversion.
    These utilities help determine the semantic meaning of status
    codes for response handling.

    @par Example
    @code
    // Check if a response should have no body
    if( statuses::is_empty( 204 ) )
    {
        // Don't send Content-Length or body
    }

    // Check if client should retry
    if( statuses::is_retry( 503 ) )
    {
        // Schedule retry with backoff
    }
    @endcode

    @see http::status, http::status_class
*/
namespace statuses {

/** Check if a status code indicates an empty response body.

    Responses with these status codes must not include a
    message body. This includes:

    @li 204 No Content
    @li 205 Reset Content  
    @li 304 Not Modified

    @param code The HTTP status code to check.

    @return `true` if responses with this code should have
    no body, `false` otherwise.
*/
BOOST_HTTP_DECL
bool
is_empty(unsigned code) noexcept;

/** Check if a status code indicates a redirect.

    Returns `true` for status codes that indicate the client
    should redirect to a different URL. This includes:

    @li 300 Multiple Choices
    @li 301 Moved Permanently
    @li 302 Found
    @li 303 See Other
    @li 305 Use Proxy
    @li 307 Temporary Redirect
    @li 308 Permanent Redirect

    Note: 304 Not Modified is not considered a redirect.

    @param code The HTTP status code to check.

    @return `true` if the code indicates a redirect,
    `false` otherwise.
*/
BOOST_HTTP_DECL
bool
is_redirect(unsigned code) noexcept;

/** Check if a status code suggests the request may be retried.

    Returns `true` for status codes that indicate a temporary
    condition where retrying the request may succeed. This includes:

    @li 502 Bad Gateway
    @li 503 Service Unavailable
    @li 504 Gateway Timeout

    @param code The HTTP status code to check.

    @return `true` if the request may be retried,
    `false` otherwise.
*/
BOOST_HTTP_DECL
bool
is_retry(unsigned code) noexcept;

} // statuses
} // http
} // boost

#endif
