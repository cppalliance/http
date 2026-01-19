//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_FRESH_HPP
#define BOOST_HTTP_SERVER_FRESH_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/request.hpp>
#include <boost/http/response.hpp>

namespace boost {
namespace http {

/** Check if a response is fresh for conditional GET.

    Compares the request's conditional headers (`If-None-Match`
    and `If-Modified-Since`) against the response's caching
    headers (`ETag` and `Last-Modified`) to determine if the
    cached response is still valid.

    If this returns `true`, the server should respond with
    304 Not Modified instead of sending the full response body.

    @par Example
    @code
    // Prepare response headers
    rp.res.set( field::etag, "\"abc123\"" );
    rp.res.set( field::last_modified, "Wed, 21 Oct 2024 07:28:00 GMT" );

    // Check freshness
    if( is_fresh( rp.req, rp.res ) )
    {
        rp.status( status::not_modified );
        co_return co_await rp.send( "" );
    }

    // Send full response
    co_return co_await rp.send( content );
    @endcode

    @param req The HTTP request.

    @param res The HTTP response (with ETag/Last-Modified set).

    @return `true` if the response is fresh (304 should be sent),
    `false` if the full response should be sent.

    @see http::field::if_none_match, http::field::if_modified_since
*/
BOOST_HTTP_DECL
bool
is_fresh(
    request const& req,
    response const& res) noexcept;

} // http
} // boost

#endif
