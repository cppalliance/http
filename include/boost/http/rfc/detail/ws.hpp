//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_DETAIL_WS_HPP
#define BOOST_HTTP_RFC_DETAIL_WS_HPP

namespace boost {
namespace http {
namespace detail {

// WS         = SP / HTAB
struct ws_t
{
    constexpr
    bool
    operator()(char c) const noexcept
    {
        return c == ' ' || c == '\t';
    }
};

constexpr ws_t ws{};

} // detail
} // http
} // boost

#endif
