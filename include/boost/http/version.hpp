//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_VERSION_HPP
#define BOOST_HTTP_VERSION_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <iosfwd>

namespace boost {
namespace http {

/** Constants representing HTTP versions.

    Only versions 1.0 and 1.1 are recognized.
*/
enum class version : char
{
    http_1_0 = 10,
    http_1_1 = 11
};

/** Return the serialized string representing the HTTP version.

    @param v The version to use.
*/
BOOST_HTTP_DECL
core::string_view
to_string(version v);

/** Format the version to an output stream.

    @return A reference to the output stream.

    @param os The output stream to write to.

    @param v The version to use.
*/
BOOST_HTTP_DECL
std::ostream&
operator<<(std::ostream& os, version v);

} // http
} // boost

#endif
