//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_MIME_TYPES_HPP
#define BOOST_HTTP_SERVER_MIME_TYPES_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <string>

namespace boost {
namespace http {

/** MIME type lookup utilities.

    This namespace provides functions for looking up MIME
    types based on file extensions, and for getting the
    default extension for a MIME type.

    @par Example
    @code
    // Get MIME type for a file
    auto type = mime_types::lookup( "index.html" );
    // type == "text/html"

    // Get MIME type for just an extension
    auto type2 = mime_types::lookup( ".css" );
    // type2 == "text/css"

    // Get full Content-Type header value
    auto ct = mime_types::content_type( "application/json" );
    // ct == "application/json; charset=utf-8"
    @endcode

    @see mime_db
*/
namespace mime_types {

/** Look up a MIME type by file path or extension.

    Given a file path or extension, returns the corresponding
    MIME type. The lookup is case-insensitive.

    @param path_or_ext A file path (e.g. "index.html") or
    extension (e.g. ".html" or "html").

    @return The MIME type string, or an empty string if
    the extension is not recognized.
*/
BOOST_HTTP_DECL
core::string_view
lookup(core::string_view path_or_ext) noexcept;

/** Return the default extension for a MIME type.

    Given a MIME type, returns the most common file
    extension associated with it.

    @param type The MIME type (e.g. "text/html").

    @return The extension without a leading dot (e.g. "html"),
    or an empty string if the type is not recognized.
*/
BOOST_HTTP_DECL
core::string_view
extension(core::string_view type) noexcept;

/** Return the default charset for a MIME type.

    @param type The MIME type (e.g. "text/html").

    @return The charset (e.g. "UTF-8"), or an empty string
    if no default charset is defined for the type.
*/
BOOST_HTTP_DECL
core::string_view
charset(core::string_view type) noexcept;

/** Build a full Content-Type header value.

    Given a MIME type or file extension, returns a complete
    Content-Type header value including charset if applicable.

    @par Example
    @code
    auto ct = mime_types::content_type( "text/html" );
    // ct == "text/html; charset=utf-8"

    auto ct2 = mime_types::content_type( ".json" );
    // ct2 == "application/json; charset=utf-8"

    auto ct3 = mime_types::content_type( "image/png" );
    // ct3 == "image/png"
    @endcode

    @param type_or_ext A MIME type or file extension.

    @return A Content-Type header value with charset if
    applicable, or an empty string if not recognized.
*/
BOOST_HTTP_DECL
std::string
content_type(core::string_view type_or_ext);

} // mime_types
} // http
} // boost

#endif
