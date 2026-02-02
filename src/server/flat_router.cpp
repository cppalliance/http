//
// Copyright (c) 2026 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/flat_router.hpp>
#include <boost/http/server/basic_router.hpp>
#include <boost/http/detail/except.hpp>

#include "src/server/detail/router_base.hpp"
#include "src/server/detail/pct_decode.hpp"
#include "src/server/detail/route_match.hpp"

#include <algorithm>

namespace boost {
namespace http {

//------------------------------------------------

struct flat_router::impl
{
    using entry = detail::router_base::entry;
    using layer = detail::router_base::layer;
    using handler = detail::router_base::handler;
    using matcher = detail::router_base::matcher;
    using opt_flags = detail::router_base::opt_flags;
    using handler_ptr = detail::router_base::handler_ptr;
    using options_handler_ptr = detail::router_base::options_handler_ptr;
    using match_result = route_params_base::match_result;

    std::vector<entry> entries;
    std::vector<matcher> matchers;

    std::uint64_t global_methods_ = 0;
    std::vector<std::string> global_custom_verbs_;
    std::string global_allow_header_;
    options_handler_ptr options_handler_;

    // RAII scope tracker sets matcher's skip_ when scope ends
    struct scope_tracker
    {
        std::vector<matcher>& matchers_;
        std::vector<entry>& entries_;
        std::size_t matcher_idx_;

        scope_tracker(
            std::vector<matcher>& m,
            std::vector<entry>& e,
            std::size_t idx)
            : matchers_(m)
            , entries_(e)
            , matcher_idx_(idx)
        {
        }

        ~scope_tracker()
        {
            matchers_[matcher_idx_].skip_ = entries_.size();
        }
    };

    // Build Allow header string from bitmask and custom verbs
    static std::string
    build_allow_header(
        std::uint64_t methods,
        std::vector<std::string> const& custom)
    {
        if(methods == ~0ULL)
            return "DELETE, GET, HEAD, OPTIONS, PATCH, POST, PUT";

        std::string result;
        // Methods in alphabetical order
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
        // Append custom verbs
        for(auto const& v : custom)
        {
            if(!result.empty())
                result += ", ";
            result += v;
        }
        return result;
    }

    static opt_flags
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
    flatten(detail::router_base::impl& src)
    {
        flatten_recursive(src, opt_flags{}, 0);
        build_allow_headers();
    }

    void
    build_allow_headers()
    {
        // Build per-matcher Allow header strings
        for(auto& m : matchers)
        {
            if(m.end_)
                m.allow_header_ = build_allow_header(
                    m.allowed_methods_, m.custom_verbs_);
        }

        // Deduplicate global custom verbs and build global Allow header
        std::sort(global_custom_verbs_.begin(), global_custom_verbs_.end());
        global_custom_verbs_.erase(
            std::unique(global_custom_verbs_.begin(), global_custom_verbs_.end()),
            global_custom_verbs_.end());
        global_allow_header_ = build_allow_header(
            global_methods_, global_custom_verbs_);
    }

    void
    flatten_recursive(
        detail::router_base::impl& src,
        opt_flags parent_opts,
        std::uint32_t depth)
    {
        opt_flags eff = compute_effective_opts(parent_opts, src.opt);

        for(auto& layer : src.layers)
        {
            // Move matcher, set flat router fields
            std::size_t matcher_idx = matchers.size();
            matchers.emplace_back(std::move(layer.match));
            auto& m = matchers.back();
            m.first_entry_ = entries.size();
            m.effective_opts_ = eff;
            m.depth_ = depth;
            // m.skip_ set by scope_tracker dtor

            scope_tracker scope(matchers, entries, matcher_idx);

            for(auto& e : layer.entries)
            {
                if(e.h->kind == detail::router_base::is_router)
                {
                    // Recurse into nested router
                    auto* nested = e.h->get_router();
                    if(nested && nested->impl_)
                        flatten_recursive(*nested->impl_, eff, depth + 1);
                }
                else
                {
                    // Collect methods for OPTIONS (only for end routes)
                    if(m.end_)
                    {
                        // Per-matcher collection
                        if(e.all)
                            m.allowed_methods_ = ~0ULL;
                        else if(e.verb != http::method::unknown)
                            m.allowed_methods_ |= (1ULL << static_cast<unsigned>(e.verb));
                        else if(!e.verb_str.empty())
                            m.custom_verbs_.push_back(e.verb_str);

                        // Global collection (for OPTIONS *)
                        if(e.all)
                            global_methods_ = ~0ULL;
                        else if(e.verb != http::method::unknown)
                            global_methods_ |= (1ULL << static_cast<unsigned>(e.verb));
                        else if(!e.verb_str.empty())
                            global_custom_verbs_.push_back(e.verb_str);
                    }

                    // Set matcher_idx, then move entire entry
                    e.matcher_idx = matcher_idx;
                    entries.emplace_back(std::move(e));
                }
            }
            // ~scope_tracker sets matchers[matcher_idx].skip_
        }
    }

    // Restore path to a given base_path length
    static void
    restore_path(
        route_params_base& p,
        std::size_t base_len)
    {
        p.base_path = { p.decoded_path_.data(), base_len };
        // Account for the addedSlash_ when computing path length
        auto const path_len = p.decoded_path_.size() - (p.addedSlash_ ? 1 : 0);
        if(base_len < path_len)
            p.path = { p.decoded_path_.data() + base_len,
                path_len - base_len };
        else
            p.path = { p.decoded_path_.data() +
                p.decoded_path_.size() - 1, 1 };  // soft slash
    }

    route_task
    dispatch_loop(route_params_base& p, bool is_options) const
    {
        // All checks happen BEFORE co_await to minimize coroutine launches.
        // Avoid touching p.ep_ (expensive atomic on Windows) - use p.kind_ for mode checks.

        std::size_t last_matched = SIZE_MAX;
        std::uint32_t current_depth = 0;
        
        // Collect methods from all matching end-route matchers for OPTIONS
        std::uint64_t options_methods = 0;
        std::vector<std::string> options_custom_verbs;

        // Stack of base_path lengths at each depth level.
        // path_stack[d] = base_path.size() before any matcher at depth d was tried.
        std::size_t path_stack[detail::router_base::max_path_depth];
        path_stack[0] = 0;

        // Track which matcher index is matched at each depth level.
        // matched_at_depth[d] = matcher index that successfully matched at depth d.
        std::size_t matched_at_depth[detail::router_base::max_path_depth];
        for(std::size_t d = 0; d < detail::router_base::max_path_depth; ++d)
            matched_at_depth[d] = SIZE_MAX;

        for(std::size_t i = 0; i < entries.size(); )
        {
            auto const& e = entries[i];
            auto const& m = matchers[e.matcher_idx];
            auto const target_depth = m.depth_;

            //--------------------------------------------------
            // Pre-invoke checks (no coroutine yet)
            //--------------------------------------------------

            // For nested routes: verify ancestors at depths 0..target_depth-1 are matched.
            // For siblings: if moving to same depth with different matcher, restore path.
            bool ancestors_ok = true;
            
            // Check if we need to match new ancestor matchers for this entry.
            // We iterate through matchers from last_matched+1 to e.matcher_idx,
            // but ONLY process those that are at depths we need (ancestors or self).
            std::size_t start_idx = (last_matched == SIZE_MAX) ? 0 : last_matched + 1;
            
            for(std::size_t check_idx = start_idx;
                check_idx <= e.matcher_idx && ancestors_ok;
                ++check_idx)
            {
                auto const& cm = matchers[check_idx];
                
                // Only check matchers that are:
                // 1. Ancestors (depth < target_depth) that we haven't matched yet, or
                // 2. The entry's own matcher
                bool is_needed_ancestor = (cm.depth_ < target_depth) && 
                    (matched_at_depth[cm.depth_] == SIZE_MAX);
                bool is_self = (check_idx == e.matcher_idx);
                
                if(!is_needed_ancestor && !is_self)
                    continue;

                // Restore path if moving to same or shallower depth
                if(cm.depth_ <= current_depth && current_depth > 0)
                {
                    restore_path(p, path_stack[cm.depth_]);
                }

                // In error/exception mode, skip end routes
                if(cm.end_ && p.kind_ != detail::router_base::is_plain)
                {
                    i = cm.skip_;
                    ancestors_ok = false;
                    break;
                }

                // Apply effective_opts for this matcher
                p.case_sensitive = (cm.effective_opts_ & 2) != 0;
                p.strict = (cm.effective_opts_ & 8) != 0;

                // Save path state before trying this matcher
                if(cm.depth_ < detail::router_base::max_path_depth)
                    path_stack[cm.depth_] = p.base_path.size();

                match_result mr;
                if(!cm(p, mr))
                {
                    // Clear matched_at_depth for this depth and deeper
                    for(std::size_t d = cm.depth_; d < detail::router_base::max_path_depth; ++d)
                        matched_at_depth[d] = SIZE_MAX;
                    i = cm.skip_;
                    ancestors_ok = false;
                    break;
                }

                // Mark this depth as matched
                if(cm.depth_ < detail::router_base::max_path_depth)
                    matched_at_depth[cm.depth_] = check_idx;
                
                last_matched = check_idx;
                current_depth = cm.depth_ + 1;

                // Save state for next depth level
                if(current_depth < detail::router_base::max_path_depth)
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

            // Check method match (only for end routes)
            if(m.end_ && !e.match_method(
                const_cast<route_params_base&>(p)))
            {
                ++i;
                continue;
            }

            // Check kind match (cheap char comparison)
            if(e.h->kind != p.kind_)
            {
                ++i;
                continue;
            }

            //--------------------------------------------------
            // Invoke handler (coroutine starts here)
            //--------------------------------------------------

            route_result rv;
            try
            {
                rv = co_await e.h->invoke(
                    const_cast<route_params_base&>(p));
            }
            catch(...)
            {
                // Only touch ep_ when actually catching
                p.ep_ = std::current_exception();
                p.kind_ = detail::router_base::is_exception;
                ++i;
                continue;
            }

            //--------------------------------------------------
            // Handle result
            //
            // Coroutines invert control - handler does the send.
            // route_what::done = handler completed request
            // route_what::next = continue to next handler
            // route_what::next_route = skip to next route
            // route_what::close = close connection
            // route_what::error = enter error mode
            //--------------------------------------------------

            if(rv.what() == route_what::next)
            {
                ++i;
                continue;
            }

            if(rv.what() == route_what::next_route)
            {
                // next_route only valid for end routes, not middleware
                if(!m.end_)
                    // VFALCO this is a logic error
                    co_return route_error(error::invalid_route_result);
                i = m.skip_;
                continue;
            }

            if(rv.what() == route_what::done ||
               rv.what() == route_what::close)
            {
                // Handler completed or requested close
                co_return rv;
            }

            // Error - transition to error mode
            p.ec_ = rv.error();
            p.kind_ = detail::router_base::is_error;

            if(m.end_)
            {
                // End routes don't have error handlers
                i = m.skip_;
                continue;
            }

            ++i;
        }

        // Final state
        if(p.kind_ == detail::router_base::is_exception)
            co_return route_error(error::unhandled_exception);
        if(p.kind_ == detail::router_base::is_error)
            co_return route_error(p.ec_);

        // OPTIONS fallback: path matched but no explicit OPTIONS handler
        if(is_options && options_methods != 0 && options_handler_)
        {
            // Build Allow header from collected methods
            std::string allow = build_allow_header(options_methods, options_custom_verbs);
            co_return co_await options_handler_->invoke(p, allow);
        }

        co_return route_next;  // no handler matched
    }
};

//------------------------------------------------

flat_router::
flat_router(
    detail::router_base&& src)
    : impl_(std::make_shared<impl>())
{
    impl_->flatten(*src.impl_);
    impl_->options_handler_ = std::move(src.options_handler_);
}

route_task
flat_router::
dispatch(
    http::method verb,
    urls::url_view const& url,
    route_params_base& p) const
{
    if(verb == http::method::unknown)
        detail::throw_invalid_argument();

    // Handle OPTIONS * before normal dispatch
    if(verb == http::method::options &&
       url.encoded_path() == "*")
    {
        if(impl_->options_handler_)
        {
            return impl_->options_handler_->invoke(
                p, impl_->global_allow_header_);
        }
        // No handler, let it fall through to 404
    }

    // Initialize params
    p.kind_ = detail::router_base::is_plain;
    p.verb_ = verb;
    p.verb_str_.clear();
    p.ec_.clear();
    p.ep_ = nullptr;
    p.decoded_path_ = detail::pct_decode_path(url.encoded_path());
    p.base_path = { p.decoded_path_.data(), 0 };
    p.path = p.decoded_path_;
    if(p.decoded_path_.back() != '/')
    {
        p.decoded_path_.push_back('/');
        p.addedSlash_ = true;
    }
    else
    {
        p.addedSlash_ = false;
    }

    return impl_->dispatch_loop(p, verb == http::method::options);
}

route_task
flat_router::
dispatch(
    std::string_view verb,
    urls::url_view const& url,
    route_params_base& p) const
{
    if(verb.empty())
        detail::throw_invalid_argument();

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
        // No handler, let it fall through to 404
    }

    // Initialize params
    p.kind_ = detail::router_base::is_plain;
    p.verb_ = method;
    if(p.verb_ == http::method::unknown)
        p.verb_str_ = verb;
    else
        p.verb_str_.clear();
    p.ec_.clear();
    p.ep_ = nullptr;
    p.decoded_path_ = detail::pct_decode_path(url.encoded_path());
    p.base_path = { p.decoded_path_.data(), 0 };
    p.path = p.decoded_path_;
    if(p.decoded_path_.back() != '/')
    {
        p.decoded_path_.push_back('/');
        p.addedSlash_ = true;
    }
    else
    {
        p.addedSlash_ = false;
    }

    return impl_->dispatch_loop(p, is_options);
}

} // http
} // boost

