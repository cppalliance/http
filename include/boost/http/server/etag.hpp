//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ETAG_HPP
#define BOOST_HTTP_SERVER_ETAG_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <cstdint>
#include <string>

namespace boost {
namespace http {

/** Options for ETag generation.
*/
struct etag_options
{
    /// Generate a weak ETag (prefixed with W/).
    bool weak = false;
};

/** Generate an ETag from content.

    Creates an ETag by computing a hash of the provided
    content. The resulting ETag can be used in HTTP
    responses to enable caching.

    @par Example
    @code
    std::string content = "Hello, World!";
    std::string tag = etag( content );
    // tag == "\"d-3/1gIbsr1bCvZ2KQgJ7DpTGR3YH\""
    @endcode

    @param body The content to hash.

    @param opts Options controlling ETag generation.

    @return An ETag string suitable for use in the ETag header.
*/
BOOST_HTTP_DECL
std::string
etag(core::string_view body, etag_options opts = {});

/** Generate an ETag from file metadata.

    Creates an ETag based on a file's size and modification
    time. This is more efficient than hashing file content
    and is suitable for static file serving.

    @par Example
    @code
    std::uint64_t size = 1234;
    std::uint64_t mtime = 1704067200; // Unix timestamp
    std::string tag = etag( size, mtime );
    // tag == "\"4d2-65956a00\""
    @endcode

    @param size The file size in bytes.

    @param mtime The file modification time (typically Unix timestamp).

    @param opts Options controlling ETag generation.

    @return An ETag string suitable for use in the ETag header.
*/
BOOST_HTTP_DECL
std::string
etag(
    std::uint64_t size,
    std::uint64_t mtime,
    etag_options opts = {});

} // http
} // boost

#endif
