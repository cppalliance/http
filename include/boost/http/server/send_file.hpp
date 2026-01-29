//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_SEND_FILE_HPP
#define BOOST_HTTP_SERVER_SEND_FILE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/router.hpp>
#include <boost/core/detail/string_view.hpp>
#include <cstdint>
#include <string>

namespace boost {
namespace http {

/** Options for sending a file.
*/
struct send_file_options
{
    /// Enable ETag generation (default: true).
    bool etag = true;

    /// Enable Last-Modified header (default: true).
    bool last_modified = true;

    /// Max-Age for Cache-Control header in seconds (0 = no cache).
    std::uint32_t max_age = 0;

    /// Content-Type to use (empty = auto-detect from extension).
    std::string content_type;
};

/** Result of send_file_init.
*/
enum class send_file_result
{
    /// File found and response prepared.
    ok,

    /// File not found.
    not_found,

    /// Response is fresh (304 Not Modified should be sent).
    not_modified,

    /// Error opening or reading file.
    error
};

/** Information about a file to send.
*/
struct send_file_info
{
    /// Result of initialization.
    send_file_result result = send_file_result::not_found;

    /// File size in bytes.
    std::uint64_t size = 0;

    /// Last modification time (system time).
    std::uint64_t mtime = 0;

    /// Content-Type to use.
    std::string content_type;

    /// ETag value.
    std::string etag;

    /// Last-Modified header value.
    std::string last_modified;

    /// Range start (for partial content).
    std::int64_t range_start = 0;

    /// Range end (for partial content).
    std::int64_t range_end = -1;

    /// True if this is a range response.
    bool is_range = false;
};

/** Initialize headers for sending a file.

    This function prepares the response headers for serving
    a static file. It performs the following tasks:

    @li Opens and validates the file
    @li Sets Content-Type based on file extension
    @li Generates ETag and Last-Modified headers
    @li Checks for conditional requests (freshness)
    @li Parses Range headers for partial content

    After calling this function, use the returned info to
    stream the file content using `res_body.write()`.

    @par Example
    @code
    route_task serve( route_params& rp, std::string path )
    {
        send_file_info info;
        send_file_init( info, rp, path );

        if( info.result == send_file_result::not_modified )
        {
            rp.status( status::not_modified );
            co_return co_await rp.send( "" );
        }

        if( info.result != send_file_result::ok )
            co_return route_next;

        // Stream file content...
    }
    @endcode

    @param info [out] Receives file information and result.

    @param rp The route parameters (for accessing request/response).

    @param path Path to the file to send.

    @param opts Options controlling file sending behavior.
*/
BOOST_HTTP_DECL
void
send_file_init(
    send_file_info& info,
    route_params& rp,
    core::string_view path,
    send_file_options const& opts = {});

/** Format Last-Modified time from Unix timestamp.

    @param mtime Unix timestamp (seconds since epoch).

    @return HTTP-date formatted string.
*/
BOOST_HTTP_DECL
std::string
format_http_date(std::uint64_t mtime);

} // http
} // boost

#endif
