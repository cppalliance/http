//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ESCAPE_HTML_HPP
#define BOOST_HTTP_SERVER_ESCAPE_HTML_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <string>

namespace boost {
namespace http {

/** Escape a string for safe inclusion in HTML.

    Replaces characters that have special meaning in HTML
    with their corresponding character entity references:

    @li `&` becomes `&amp;`
    @li `<` becomes `&lt;`
    @li `>` becomes `&gt;`
    @li `"` becomes `&quot;`
    @li `'` becomes `&#39;`

    This function is used to prevent XSS (Cross-Site Scripting)
    attacks when embedding user input in HTML responses.

    @par Example
    @code
    std::string safe = escape_html( "<script>alert('xss')</script>" );
    // safe == "&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;"
    @endcode

    @param s The string to escape.

    @return A new string with HTML special characters escaped.
*/
BOOST_HTTP_DECL
std::string
escape_html(core::string_view s);

} // http
} // boost

#endif
