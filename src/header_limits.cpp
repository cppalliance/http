//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/header_limits.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/http/detail/header.hpp>

namespace boost {
namespace http {

std::size_t
header_limits::
valid_space_needed() const
{
/*
    // "HTTP/1.1 200 OK\r\n\r\n" = 19
    // "X / HTTP/1.1\r\n"        = 14
    // "HTTP/1.1 200\r\n"        = 14
    // "X:\r\n"                  =  4

    // make sure `size` is big enough
    // to hold the largest default buffer:
    //if( max_size < 19)
        //max_size = 19;

    // max_size too small
    if( max_size < 19)
        detail::throw_invalid_argument();

    // max_size too large
    if( max_size >
        BOOST_HTTP_MAX_HEADER)
        detail::throw_invalid_argument();

    // max_start_line too small
    if( max_start_line < 14)
        detail::throw_invalid_argument();

    // max_start_line too large
    if( max_start_line >
        max_size - 2)
        detail::throw_invalid_argument();

    // max_field too small
    if( max_field < 4)
        detail::throw_invalid_argument();

    // max_field too large
    if( max_field >
        max_size)
        detail::throw_invalid_argument();

    // max_fields too large
    if( max_fields >
        max_size / 4)
        detail::throw_invalid_argument();
*/
    static constexpr auto Align =
        alignof(detail::header::entry);
    // round up to alignof(A)
    return Align * (
        (max_size + Align - 1) / Align) + (
            max_fields * sizeof(
                detail::header::entry));
}

} // http
} // boost
