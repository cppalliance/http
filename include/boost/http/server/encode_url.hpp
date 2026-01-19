//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ENCODE_URL_HPP
#define BOOST_HTTP_SERVER_ENCODE_URL_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <string>

namespace boost {
namespace http {

/** Percent-encode a URL for safe use in HTTP responses.

    Encodes characters that are not safe in URLs using
    percent-encoding (e.g. space becomes %20). This is
    useful for encoding URLs that will be included in
    Location headers or HTML links.

    The following characters are NOT encoded:
    - Unreserved: A-Z a-z 0-9 - _ . ~
    - Reserved (allowed in URLs): ! # $ & ' ( ) * + , / : ; = ? @

    @par Example
    @code
    std::string url = encode_url( "/path/to/file with spaces.txt" );
    // url == "/path/to/file%20with%20spaces.txt"
    @endcode

    @param url The URL or URL component to encode.

    @return A new string with unsafe characters percent-encoded.
*/
BOOST_HTTP_DECL
std::string
encode_url(core::string_view url);

} // http
} // boost

#endif
