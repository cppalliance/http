//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_MIME_DB_HPP
#define BOOST_HTTP_SERVER_MIME_DB_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>

namespace boost {
namespace http {

/** Information about a MIME type.

    This structure contains metadata associated with a
    MIME type, including its default charset and whether
    the content is typically compressible.
*/
struct mime_type_entry
{
    /// The MIME type string (e.g. "text/html").
    core::string_view type;

    /// Default charset, or empty if none.
    core::string_view charset;

    /// Whether content of this type is typically compressible.
    bool compressible = false;
};

/** MIME type database utilities.

    This namespace provides lookup functions for the
    built-in MIME type database. The database contains
    common MIME types with associated metadata like
    default charset and compressibility.

    @par Example
    @code
    // Look up info about a MIME type
    auto const* entry = mime_db::lookup( "text/html" );
    if( entry )
    {
        std::cout << "charset: " << entry->charset << "\n";
        std::cout << "compressible: " << entry->compressible << "\n";
    }
    @endcode

    @see mime_types
*/
namespace mime_db {

/** Look up a MIME type in the database.

    Searches the built-in MIME type database for the
    specified type string. The lookup is case-insensitive.

    @param type The MIME type to look up (e.g. "text/html").

    @return A pointer to the entry if found, or `nullptr`
    if the type is not in the database.
*/
BOOST_HTTP_DECL
mime_type_entry const*
lookup(core::string_view type) noexcept;

} // mime_db
} // http
} // boost

#endif
