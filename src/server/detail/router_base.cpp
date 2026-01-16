//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/server/detail/router_base.hpp"
#include <boost/http/server/detail/router_base.hpp>
#include <boost/http/server/flat_router.hpp>
#include <boost/http/server/route_handler.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include "src/server/detail/route_match.hpp"
#include "src/server/detail/route_rule.hpp"

/*

pattern     target      path(use)    path(get) 
-------------------------------------------------
/           /           /
/           /api        /api
/api        /api        /            /api
/api        /api/       /            /api/
/api        /api/       /            no-match       strict
/api        /api/v0     /v0          no-match
/api/       /api        /            /api
/api/       /api        /            no-match       strict
/api/       /api/       /            /api/
/api/       /api/v0     /v0          no-match

*/

namespace boost {
namespace http {
namespace detail {

router_base::
~router_base()
{
    delete impl_;
}

router_base::
router_base(
    opt_flags opt)
    : impl_(new impl(opt))
{
}

router_base::
router_base(
    router_base&& other) noexcept
    :impl_(other.impl_)
{
    other.impl_ = nullptr;
}

router_base&
router_base::
operator=(
    router_base&& other) noexcept
{
    delete impl_;
    impl_ = other.impl_;
    other.impl_ = 0;
    return *this;
}

auto
router_base::
new_layer(
    std::string_view pattern) -> layer&
{
    // the pattern must not be empty
    if(pattern.empty())
        detail::throw_invalid_argument();
    // delete the last route if it is empty,
    // this happens if they call route() without
    // adding anything
    if(! impl_->layers.empty() &&
        impl_->layers.back().entries.empty())
        impl_->layers.pop_back();
    impl_->layers.emplace_back(pattern);
    return impl_->layers.back();
};

std::size_t
router_base::
new_layer_idx(
    std::string_view pattern)
{
    new_layer(pattern);
    return impl_->layers.size() - 1;
}

auto
router_base::
get_layer(
    std::size_t idx) -> layer&
{
    return impl_->layers[idx];
}

void
router_base::
add_impl(
    std::string_view pattern,
    handlers hn)
{
    if( pattern.empty())
        pattern = "/";
    impl_->layers.emplace_back(
        pattern, hn);

    // Validate depth for any nested routers
    auto& lay = impl_->layers.back();
    for(auto& entry : lay.entries)
        if(entry.h->kind == is_router)
            if(auto* r = entry.h->get_router())
                r->set_nested_depth(impl_->depth_);
}

void
router_base::
add_impl(
    layer& l,
    http::method verb,
    handlers hn)
{
    // cannot be unknown
    if(verb == http::method::unknown)
        detail::throw_invalid_argument();

    l.entries.reserve(l.entries.size() + hn.n);
    for(std::size_t i = 0; i < hn.n; ++i)
        l.entries.emplace_back(verb,
            std::move(hn.p[i]));
}

void
router_base::
add_impl(
    layer& l,
    std::string_view verb_str,
    handlers hn)
{
    l.entries.reserve(l.entries.size() + hn.n);

    if(verb_str.empty())
    {
        // all
        for(std::size_t i = 0; i < hn.n; ++i)
            l.entries.emplace_back(std::move(hn.p[i]));
        return;
    }

    // possibly custom string
    for(std::size_t i = 0; i < hn.n; ++i)
        l.entries.emplace_back(verb_str,
            std::move(hn.p[i]));
}

void
router_base::
set_nested_depth(
    std::size_t parent_depth)
{
    std::size_t d = parent_depth + 1;
    if(d >= max_path_depth)
        detail::throw_length_error(
            "router nesting depth exceeds max_path_depth");
    impl_->depth_ = d;
    for(auto& layer : impl_->layers)
        for(auto& entry : layer.entries)
            if(entry.h->kind == is_router)
                if(auto* r = entry.h->get_router())
                    r->set_nested_depth(d);
}

} // detail
} // http
} // boost

