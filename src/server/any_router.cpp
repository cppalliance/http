//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/server/detail/any_router.hpp"
#include <boost/http/server/any_router.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/http/error.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include "src/server/detail/pct_decode.hpp"

#include <algorithm>

namespace boost {
namespace http {

//------------------------------------------------
//
// impl helpers
//
//------------------------------------------------

std::string
any_router::impl::
build_allow_header(
    std::uint64_t methods,
    std::vector<std::string> const& custom)
{
    if(methods == ~0ULL)
        return "DELETE, GET, HEAD, OPTIONS, PATCH, POST, PUT";

    std::string result;
    static constexpr std::pair<http::method, char const*> known[] = {
        {http::method::acl, "ACL"},
        {http::method::bind, "BIND"},
        {http::method::checkout, "CHECKOUT"},
        {http::method::connect, "CONNECT"},
        {http::method::copy, "COPY"},
        {http::method::delete_, "DELETE"},
        {http::method::get, "GET"},
        {http::method::head, "HEAD"},
        {http::method::link, "LINK"},
        {http::method::lock, "LOCK"},
        {http::method::merge, "MERGE"},
        {http::method::mkactivity, "MKACTIVITY"},
        {http::method::mkcalendar, "MKCALENDAR"},
        {http::method::mkcol, "MKCOL"},
        {http::method::move, "MOVE"},
        {http::method::msearch, "M-SEARCH"},
        {http::method::notify, "NOTIFY"},
        {http::method::options, "OPTIONS"},
        {http::method::patch, "PATCH"},
        {http::method::post, "POST"},
        {http::method::propfind, "PROPFIND"},
        {http::method::proppatch, "PROPPATCH"},
        {http::method::purge, "PURGE"},
        {http::method::put, "PUT"},
        {http::method::rebind, "REBIND"},
        {http::method::report, "REPORT"},
        {http::method::search, "SEARCH"},
        {http::method::subscribe, "SUBSCRIBE"},
        {http::method::trace, "TRACE"},
        {http::method::unbind, "UNBIND"},
        {http::method::unlink, "UNLINK"},
        {http::method::unlock, "UNLOCK"},
        {http::method::unsubscribe, "UNSUBSCRIBE"},
    };
    for(auto const& [m, name] : known)
    {
        if(methods & (1ULL << static_cast<unsigned>(m)))
        {
            if(!result.empty())
                result += ", ";
            result += name;
        }
    }
    for(auto const& v : custom)
    {
        if(!result.empty())
            result += ", ";
        result += v;
    }
    return result;
}

any_router::opt_flags
any_router::impl::
compute_effective_opts(
    opt_flags parent,
    opt_flags child)
{
    opt_flags result = parent;

    // case_sensitive: bits 1-2 (2=true, 4=false)
    if(child & 2)
        result = (result & ~6) | 2;
    else if(child & 4)
        result = (result & ~6) | 4;

    // strict: bits 3-4 (8=true, 16=false)
    if(child & 8)
        result = (result & ~24) | 8;
    else if(child & 16)
        result = (result & ~24) | 16;

    return result;
}

void
any_router::impl::
restore_path(
    route_params_base& p,
    std::size_t base_len)
{
    p.base_path = { p.decoded_path_.data(), base_len };
    auto const path_len = p.decoded_path_.size() - (p.addedSlash_ ? 1 : 0);
    if(base_len < path_len)
        p.path = { p.decoded_path_.data() + base_len,
            path_len - base_len };
    else
        p.path = { p.decoded_path_.data() +
            p.decoded_path_.size() - 1, 1 };  // soft slash
}

void
any_router::impl::
update_allow_for_entry(
    matcher& m,
    entry const& e)
{
    if(!m.end_)
        return;

    // Per-matcher collection
    if(e.all)
        m.allowed_methods_ = ~0ULL;
    else if(e.verb != http::method::unknown)
        m.allowed_methods_ |= (1ULL << static_cast<unsigned>(e.verb));
    else if(!e.verb_str.empty())
        m.custom_verbs_.push_back(e.verb_str);

    // Rebuild per-matcher Allow header eagerly
    m.allow_header_ = build_allow_header(
        m.allowed_methods_, m.custom_verbs_);

    // Global collection (for OPTIONS *)
    if(e.all)
        global_methods_ = ~0ULL;
    else if(e.verb != http::method::unknown)
        global_methods_ |= (1ULL << static_cast<unsigned>(e.verb));
    else if(!e.verb_str.empty())
        global_custom_verbs_.push_back(e.verb_str);
}

void
any_router::impl::
rebuild_global_allow_header()
{
    std::sort(global_custom_verbs_.begin(), global_custom_verbs_.end());
    global_custom_verbs_.erase(
        std::unique(global_custom_verbs_.begin(), global_custom_verbs_.end()),
        global_custom_verbs_.end());
    global_allow_header_ = build_allow_header(
        global_methods_, global_custom_verbs_);
}

void
any_router::impl::
finalize_pending()
{
    if(pending_route_ == SIZE_MAX)
        return;
    auto& m = matchers[pending_route_];
    if(entries.size() == m.first_entry_)
    {
        // empty route, remove it
        matchers.pop_back();
    }
    else
    {
        m.skip_ = entries.size();
    }
    pending_route_ = SIZE_MAX;
}

//------------------------------------------------
//
// dispatch
//
//------------------------------------------------

route_task
any_router::impl::
dispatch_loop(route_params_base& p, bool is_options) const
{
    std::size_t last_matched = SIZE_MAX;
    std::uint32_t current_depth = 0;

    std::uint64_t options_methods = 0;
    std::vector<std::string> options_custom_verbs;

    std::size_t path_stack[any_router::max_path_depth];
    path_stack[0] = 0;

    std::size_t matched_at_depth[any_router::max_path_depth];
    for(std::size_t d = 0; d < any_router::max_path_depth; ++d)
        matched_at_depth[d] = SIZE_MAX;

    for(std::size_t i = 0; i < entries.size(); )
    {
        auto const& e = entries[i];
        auto const& m = matchers[e.matcher_idx];
        auto const target_depth = m.depth_;

        bool ancestors_ok = true;

        std::size_t start_idx = (last_matched == SIZE_MAX) ? 0 : last_matched + 1;

        for(std::size_t check_idx = start_idx;
            check_idx <= e.matcher_idx && ancestors_ok;
            ++check_idx)
        {
            auto const& cm = matchers[check_idx];

            bool is_needed_ancestor = (cm.depth_ < target_depth) &&
                (matched_at_depth[cm.depth_] == SIZE_MAX);
            bool is_self = (check_idx == e.matcher_idx);

            if(!is_needed_ancestor && !is_self)
                continue;

            if(cm.depth_ <= current_depth && current_depth > 0)
            {
                restore_path(p, path_stack[cm.depth_]);
            }

            if(cm.end_ && p.kind_ != any_router::is_plain)
            {
                i = cm.skip_;
                ancestors_ok = false;
                break;
            }

            p.case_sensitive = (cm.effective_opts_ & 2) != 0;
            p.strict = (cm.effective_opts_ & 8) != 0;

            if(cm.depth_ < any_router::max_path_depth)
                path_stack[cm.depth_] = p.base_path.size();

            match_result mr;
            if(!cm(p, mr))
            {
                for(std::size_t d = cm.depth_; d < any_router::max_path_depth; ++d)
                    matched_at_depth[d] = SIZE_MAX;
                i = cm.skip_;
                ancestors_ok = false;
                break;
            }

            if(!mr.params_.empty())
            {
                for(auto& param : mr.params_)
                    p.params.push_back(std::move(param));
            }

            if(cm.depth_ < any_router::max_path_depth)
                matched_at_depth[cm.depth_] = check_idx;

            last_matched = check_idx;
            current_depth = cm.depth_ + 1;

            if(current_depth < any_router::max_path_depth)
                path_stack[current_depth] = p.base_path.size();
        }

        if(!ancestors_ok)
            continue;

        // Collect methods from matching end-route matchers for OPTIONS
        if(is_options && m.end_)
        {
            options_methods |= m.allowed_methods_;
            for(auto const& v : m.custom_verbs_)
                options_custom_verbs.push_back(v);
        }

        if(m.end_ && !e.match_method(
            const_cast<route_params_base&>(p)))
        {
            ++i;
            continue;
        }

        if(e.h->kind != p.kind_)
        {
            ++i;
            continue;
        }

        //--------------------------------------------------
        // Invoke handler
        //--------------------------------------------------

        route_result rv;
        try
        {
            rv = co_await e.h->invoke(
                const_cast<route_params_base&>(p));
        }
        catch(...)
        {
            p.ep_ = std::current_exception();
            p.kind_ = any_router::is_exception;
            ++i;
            continue;
        }

        if(rv.what() == route_what::next)
        {
            ++i;
            continue;
        }

        if(rv.what() == route_what::next_route)
        {
            if(!m.end_)
                co_return route_error(error::invalid_route_result);
            i = m.skip_;
            continue;
        }

        if(rv.what() == route_what::done ||
           rv.what() == route_what::close)
        {
            co_return rv;
        }

        // Error - transition to error mode
        p.ec_ = rv.error();
        p.kind_ = any_router::is_error;

        if(m.end_)
        {
            i = m.skip_;
            continue;
        }

        ++i;
    }

    if(p.kind_ == any_router::is_exception)
        co_return route_error(error::unhandled_exception);
    if(p.kind_ == any_router::is_error)
        co_return route_error(p.ec_);

    // OPTIONS fallback
    if(is_options && options_methods != 0 && options_handler_)
    {
        std::string allow = build_allow_header(options_methods, options_custom_verbs);
        co_return co_await options_handler_->invoke(p, allow);
    }

    co_return route_next;
}

//------------------------------------------------
//
// any_router
//
//------------------------------------------------

any_router::
any_router(
    opt_flags opt)
    : impl_(std::make_shared<impl>(opt))
{
}

void
any_router::
add_middleware(
    std::string_view pattern,
    handlers hn)
{
    impl_->finalize_pending();

    if(pattern.empty())
        pattern = "/";

    auto const matcher_idx = impl_->matchers.size();
    impl_->matchers.emplace_back(pattern, false);
    auto& m = impl_->matchers.back();
    if(m.error())
        detail::throw_invalid_argument();
    m.first_entry_ = impl_->entries.size();
    m.effective_opts_ = impl::compute_effective_opts(0, impl_->opt_);
    m.own_opts_ = impl_->opt_;
    m.depth_ = 0;

    for(std::size_t i = 0; i < hn.n; ++i)
    {
        impl_->entries.emplace_back(std::move(hn.p[i]));
        impl_->entries.back().matcher_idx = matcher_idx;
    }

    m.skip_ = impl_->entries.size();
}

void
any_router::
inline_router(
    std::string_view pattern,
    any_router&& sub)
{
    impl_->finalize_pending();

    if(!sub.impl_)
        return;

    sub.impl_->finalize_pending();

    if(pattern.empty())
        pattern = "/";

    // Create parent matcher for the mount point
    auto const parent_matcher_idx = impl_->matchers.size();
    impl_->matchers.emplace_back(pattern, false);
    auto& parent_m = impl_->matchers.back();
    if(parent_m.error())
        detail::throw_invalid_argument();
    parent_m.first_entry_ = impl_->entries.size();

    auto parent_eff = impl::compute_effective_opts(0, impl_->opt_);
    parent_m.effective_opts_ = parent_eff;
    parent_m.own_opts_ = impl_->opt_;
    parent_m.depth_ = 0;

    // Check nesting depth
    std::size_t max_sub_depth = 0;
    for(auto const& sm : sub.impl_->matchers)
        max_sub_depth = (std::max)(max_sub_depth,
            static_cast<std::size_t>(sm.depth_));
    if(max_sub_depth + 1 >= max_path_depth)
        detail::throw_length_error(
            "router nesting depth exceeds max_path_depth");

    // Compute offsets for re-indexing
    auto const matcher_offset = impl_->matchers.size();
    auto const entry_offset = impl_->entries.size();

    // Recompute effective_opts for inlined matchers using depth stack
    auto sub_root_eff = impl::compute_effective_opts(
        parent_eff, sub.impl_->opt_);
    opt_flags eff_stack[max_path_depth];
    eff_stack[0] = sub_root_eff;

    // Inline sub's matchers
    for(auto& sm : sub.impl_->matchers)
    {
        auto d = sm.depth_;
        opt_flags parent = (d > 0) ? eff_stack[d - 1] : parent_eff;
        eff_stack[d] = impl::compute_effective_opts(parent, sm.own_opts_);
        sm.effective_opts_ = eff_stack[d];
        sm.depth_ += 1;  // increase by 1 (parent is at depth 0)
        sm.first_entry_ += entry_offset;
        sm.skip_ += entry_offset;
        impl_->matchers.push_back(std::move(sm));
    }

    // Inline sub's entries
    for(auto& se : sub.impl_->entries)
    {
        se.matcher_idx += matcher_offset;
        impl_->entries.push_back(std::move(se));
    }

    // Set parent matcher's skip
    // Need to re-fetch since vector may have reallocated
    impl_->matchers[parent_matcher_idx].skip_ = impl_->entries.size();

    // Merge global methods
    impl_->global_methods_ |= sub.impl_->global_methods_;
    for(auto& v : sub.impl_->global_custom_verbs_)
        impl_->global_custom_verbs_.push_back(std::move(v));
    impl_->rebuild_global_allow_header();

    // Move options handler if sub has one and parent doesn't
    if(sub.impl_->options_handler_ && !impl_->options_handler_)
        impl_->options_handler_ = std::move(sub.impl_->options_handler_);

    sub.impl_.reset();
}

std::size_t
any_router::
new_route(
    std::string_view pattern)
{
    impl_->finalize_pending();

    if(pattern.empty())
        detail::throw_invalid_argument();

    auto const idx = impl_->matchers.size();
    impl_->matchers.emplace_back(pattern, true);
    auto& m = impl_->matchers.back();
    if(m.error())
        detail::throw_invalid_argument();
    m.first_entry_ = impl_->entries.size();
    m.effective_opts_ = impl::compute_effective_opts(0, impl_->opt_);
    m.own_opts_ = impl_->opt_;
    m.depth_ = 0;

    impl_->pending_route_ = idx;
    return idx;
}

void
any_router::
add_to_route(
    std::size_t idx,
    http::method verb,
    handlers hn)
{
    if(verb == http::method::unknown)
        detail::throw_invalid_argument();

    auto& m = impl_->matchers[idx];
    for(std::size_t i = 0; i < hn.n; ++i)
    {
        impl_->entries.emplace_back(verb, std::move(hn.p[i]));
        impl_->entries.back().matcher_idx = idx;
        impl_->update_allow_for_entry(m, impl_->entries.back());
    }
    impl_->rebuild_global_allow_header();
}

void
any_router::
add_to_route(
    std::size_t idx,
    std::string_view verb,
    handlers hn)
{
    auto& m = impl_->matchers[idx];

    if(verb.empty())
    {
        // all methods
        for(std::size_t i = 0; i < hn.n; ++i)
        {
            impl_->entries.emplace_back(std::move(hn.p[i]));
            impl_->entries.back().matcher_idx = idx;
            impl_->update_allow_for_entry(m, impl_->entries.back());
        }
    }
    else
    {
        // specific method string
        for(std::size_t i = 0; i < hn.n; ++i)
        {
            impl_->entries.emplace_back(verb, std::move(hn.p[i]));
            impl_->entries.back().matcher_idx = idx;
            impl_->update_allow_for_entry(m, impl_->entries.back());
        }
    }
    impl_->rebuild_global_allow_header();
}

void
any_router::
finalize_pending()
{
    if(impl_)
        impl_->finalize_pending();
}

void
any_router::
set_options_handler_impl(
    options_handler_ptr p)
{
    impl_->options_handler_ = std::move(p);
}

//------------------------------------------------
//
// dispatch
//
//------------------------------------------------

route_task
any_router::
dispatch(
    http::method verb,
    urls::url_view const& url,
    route_params_base& p) const
{
    if(verb == http::method::unknown)
        detail::throw_invalid_argument();

    impl_->ensure_finalized();

    // Handle OPTIONS * before normal dispatch
    if(verb == http::method::options &&
       url.encoded_path() == "*")
    {
        if(impl_->options_handler_)
        {
            return impl_->options_handler_->invoke(
                p, impl_->global_allow_header_);
        }
    }

    // Initialize params
    p.kind_ = is_plain;
    p.verb_ = verb;
    p.verb_str_.clear();
    p.ec_.clear();
    p.ep_ = nullptr;
    p.params.clear();
    p.decoded_path_ = detail::pct_decode_path(url.encoded_path());
    if(p.decoded_path_.empty() || p.decoded_path_.back() != '/')
    {
        p.decoded_path_.push_back('/');
        p.addedSlash_ = true;
    }
    else
    {
        p.addedSlash_ = false;
    }
    p.base_path = { p.decoded_path_.data(), 0 };
    auto const subtract = (p.addedSlash_ && p.decoded_path_.size() > 1) ? 1 : 0;
    p.path = { p.decoded_path_.data(), p.decoded_path_.size() - subtract };

    return impl_->dispatch_loop(p, verb == http::method::options);
}

route_task
any_router::
dispatch(
    std::string_view verb,
    urls::url_view const& url,
    route_params_base& p) const
{
    if(verb.empty())
        detail::throw_invalid_argument();

    impl_->ensure_finalized();

    auto const method = http::string_to_method(verb);
    bool const is_options = (method == http::method::options);

    // Handle OPTIONS * before normal dispatch
    if(is_options && url.encoded_path() == "*")
    {
        if(impl_->options_handler_)
        {
            return impl_->options_handler_->invoke(
                p, impl_->global_allow_header_);
        }
    }

    // Initialize params
    p.kind_ = is_plain;
    p.verb_ = method;
    if(p.verb_ == http::method::unknown)
        p.verb_str_ = verb;
    else
        p.verb_str_.clear();
    p.ec_.clear();
    p.ep_ = nullptr;
    p.params.clear();
    p.decoded_path_ = detail::pct_decode_path(url.encoded_path());
    if(p.decoded_path_.empty() || p.decoded_path_.back() != '/')
    {
        p.decoded_path_.push_back('/');
        p.addedSlash_ = true;
    }
    else
    {
        p.addedSlash_ = false;
    }
    p.base_path = { p.decoded_path_.data(), 0 };
    auto const subtract = (p.addedSlash_ && p.decoded_path_.size() > 1) ? 1 : 0;
    p.path = { p.decoded_path_.data(), p.decoded_path_.size() - subtract };

    return impl_->dispatch_loop(p, is_options);
}

} // http
} // boost
