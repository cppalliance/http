//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_BCRYPT_VERSION_HPP
#define BOOST_HTTP_BCRYPT_VERSION_HPP

#include <boost/http/detail/config.hpp>

namespace boost {
namespace http {
namespace bcrypt {

/** bcrypt hash version prefix.

    The version determines which variant of bcrypt is used.
    All versions produce compatible hashes.
*/
enum class version
{
    /// $2a$ - Original specification
    v2a,

    /// $2b$ - Fixed handling of passwords > 255 chars (recommended)
    v2b
};

} // bcrypt
} // http
} // boost

#endif
