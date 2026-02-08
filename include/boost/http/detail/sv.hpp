//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_SV_HPP
#define BOOST_HTTP_DETAIL_SV_HPP

#include <boost/core/detail/string_view.hpp>

namespace boost {
namespace http {
namespace detail {

// Pulling in the UDL directly breaks in some places on MSVC,
// so introduce a namespace for this purprose.
namespace string_literals {
inline
core::string_view
operator""_sv(
    char const* p,
    std::size_t n) noexcept
{
    return core::string_view(p, n);
}
} // string_literals

} // detail
} // http
} // boost

#endif
