//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZLIB_ERROR_HPP
#define BOOST_HTTP_ZLIB_ERROR_HPP

#include <boost/http/detail/config.hpp>

namespace boost {
namespace http {
namespace zlib {

/** Error codes returned from compression/decompression functions.

    Negative values are errors, positive values are used
    for special but normal events.
*/
enum class error
{
    ok          =  0,
    stream_end  =  1,
    need_dict   =  2,
    errno_      = -1,
    stream_err  = -2,
    data_err    = -3,
    mem_err     = -4,
    buf_err     = -5,
    version_err = -6
};

} // zlib
} // http
} // boost

#include <boost/http/zlib/impl/error.hpp>

#endif
