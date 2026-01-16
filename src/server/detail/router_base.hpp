//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SRC_SERVER_DETAIL_ROUTER_BASE_HPP
#define BOOST_HTTP_SRC_SERVER_DETAIL_ROUTER_BASE_HPP

#include <boost/http/server/detail/router_base.hpp>
#include "src/server/detail/route_match.hpp"

namespace boost {
namespace http {
namespace detail {

// An entry describes a single route handler.
// This can be an end route or a middleware.
// Members ordered largest-to-smallest for optimal packing.
struct router_base::entry
{
    // ~32 bytes (SSO string)
    std::string verb_str;

    // 8 bytes each
    handler_ptr h;
    std::size_t matcher_idx = 0;    // flat_router: index into matchers vector

    // 4 bytes
    http::method verb = http::method::unknown;

    // 1 byte (+ 3 bytes padding)
    bool all;

    // all
    explicit entry(
        handler_ptr h_) noexcept
        : h(std::move(h_))
        , all(true)
    {
    }

    // verb match
    entry(
        http::method verb_,
        handler_ptr h_) noexcept
        : h(std::move(h_))
        , verb(verb_)
        , all(false)
    {
        BOOST_ASSERT(verb !=
            http::method::unknown);
    }

    // verb match
    entry(
        std::string_view verb_str_,
        handler_ptr h_) noexcept
        : h(std::move(h_))
        , verb(http::string_to_method(verb_str_))
        , all(false)
    {
        if(verb != http::method::unknown)
            return;
        verb_str = verb_str_;
    }

    bool match_method(
        route_params_base& rp) const noexcept
    {
        detail::route_params_access RP{rp};
        if(all)
            return true;
        if(verb != http::method::unknown)
            return RP->verb_ == verb;
        if(RP->verb_ != http::method::unknown)
            return false;
        return RP->verb_str_ == verb_str;
    }
};

// A layer is a set of entries that match a route
struct router_base::layer
{
    matcher match;
    std::vector<entry> entries;

    // middleware layer
    layer(
        std::string_view pat,
        handlers hn)
        : match(pat, false)
    {
        entries.reserve(hn.n);
        for(std::size_t i = 0; i < hn.n; ++i)
            entries.emplace_back(std::move(hn.p[i]));
    }

    // route layer
    explicit layer(
        std::string_view pat)
        : match(pat, true)
    {
    }
};

struct router_base::impl
{
    std::vector<layer> layers;
    opt_flags opt;
    std::size_t depth_ = 0;

    explicit impl(
        opt_flags opt_) noexcept
        : opt(opt_)
    {
    }
};

} // detail
} // http
} // boost

#endif
