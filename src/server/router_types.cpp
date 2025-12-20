//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/server/router_types.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/assert.hpp>
#include <cstring>

namespace boost {
namespace http_proto {

namespace detail {

const char*
route_cat_type::
name() const noexcept
{
    return "boost.http";
}

std::string
route_cat_type::
message(int code) const
{
    return message(code, nullptr, 0);
}

char const*
route_cat_type::
message(
    int code,
    char*,
    std::size_t) const noexcept
{
    switch(static_cast<route>(code))
    {
    case route::close:      return "route::close";
    case route::complete:   return "route::complete";
    case route::suspend:     return "route::suspend";
    case route::next:       return "route::next";
    case route::next_route: return "route::next_route";
    case route::send:       return "route::send";
    default:
        return "?";
    }
}

// msvc 14.0 has a bug that warns about inability
// to use constexpr construction here, even though
// there's no constexpr construction
#if defined(_MSC_VER) && _MSC_VER <= 1900
# pragma warning( push )
# pragma warning( disable : 4592 )
#endif

#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
constinit route_cat_type route_cat;
#else
route_cat_type route_cat;
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1900
# pragma warning( pop )
#endif

} // detail

resumer
suspender::
owner::
do_suspend()
{
    detail::throw_logic_error();
}

bool
route_params_base::
is_method(
    core::string_view s) const noexcept
{
    auto m = http_proto::string_to_method(s);
    if(m != http_proto::method::unknown)
        return verb_ == m;
    return s == verb_str_;
}

} // http_proto
} // boost
