//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/server/detail/pct_decode.hpp"

namespace boost {
namespace http {
namespace detail {

bool
ci_is_equal(
    core::string_view s0,
    core::string_view s1) noexcept
{
    auto n = s0.size();
    if(s1.size() != n)
        return false;
    auto p1 = s0.data();
    auto p2 = s1.data();
    char a, b;
    // fast loop
    while(n--)
    {
        a = *p1++;
        b = *p2++;
        if(a != b)
            goto slow;
    }
    return true;
    do
    {
        a = *p1++;
        b = *p2++;
    slow:
        if( grammar::to_lower(a) !=
            grammar::to_lower(b))
            return false;
    }
    while(n--);
    return true;
}

std::string
pct_decode(
    urls::pct_string_view s)
{
    std::string result;
    core::string_view sv(s);
    result.reserve(s.size());
    auto it = sv.data();
    auto const end = it + sv.size();
    for(;;)
    {
        if(it == end)
            break;
        if(*it != '%')
        {
            result.push_back(*it++);
            continue;
        }
        ++it;
#if 0
        // pct_string_view can never have invalid pct-encodings
        if(it == end)
            goto invalid;
#endif
        auto d0 = urls::grammar::hexdig_value(*it++);
#if 0
        // pct_string_view can never have invalid pct-encodings
        if( d0 < 0 ||
            it == end)
            goto invalid;
#endif
        auto d1 = urls::grammar::hexdig_value(*it++);
#if 0
        // pct_string_view can never have invalid pct-encodings
        if(d1 < 0)
            goto invalid;
#endif
        result.push_back(d0 * 16 + d1);
    }
    return result;
#if 0
invalid:
    // can't get here, as received a pct_string_view
    detail::throw_invalid_argument();
#endif
}

// decode all percent escapes except slashes '/' and '\'
std::string
pct_decode_path(
    urls::pct_string_view s)
{
    std::string result;
    core::string_view sv(s);
    result.reserve(s.size());
    auto it = sv.data();
    auto const end = it + sv.size();
    for(;;)
    {
        if(it == end)
            break;
        if(*it != '%')
        {
            result.push_back(*it++);
            continue;
        }
        ++it;
#if 0
        // pct_string_view can never have invalid pct-encodings
        if(it == end)
            goto invalid;
#endif
        auto d0 = urls::grammar::hexdig_value(*it++);
#if 0
        // pct_string_view can never have invalid pct-encodings
        if( d0 < 0 ||
            it == end)
            goto invalid;
#endif
        auto d1 = urls::grammar::hexdig_value(*it++);
#if 0
        // pct_string_view can never have invalid pct-encodings
        if(d1 < 0)
            goto invalid;
#endif
        char c = d0 * 16 + d1;
        if( c != '/' &&
            c != '\\')
        {
            result.push_back(c);
            continue;
        }
        result.append(it - 3, 3);
    }
    return result;
#if 0
invalid:
    // can't get here, as received a pct_string_view
    detail::throw_invalid_argument();
#endif
}

} // detail
} // http
} // boost
