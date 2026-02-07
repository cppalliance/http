//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SRC_SERVER_DETAIL_ANY_ROUTER_HPP
#define BOOST_HTTP_SRC_SERVER_DETAIL_ANY_ROUTER_HPP

#include <boost/http/server/detail/router_base.hpp>
#include <boost/http/detail/except.hpp>
#include "src/server/detail/route_match.hpp"
#include <mutex>

namespace boost {
namespace http {
namespace detail {

struct router_base::entry
{
    // ~32 bytes (SSO string)
    std::string verb_str;

    // 8 bytes each
    handler_ptr h;
    std::size_t matcher_idx = 0;

    // 4 bytes
    http::method verb = http::method::unknown;

    // 1 byte (+ 3 bytes padding)
    bool all;

    // all methods
    explicit entry(
        handler_ptr h_) noexcept
        : h(std::move(h_))
        , all(true)
    {
    }

    // known verb match
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

    // string verb match
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
        route_params& rp) const noexcept
    {
        route_params_access RP{rp};
        if(all)
            return true;
        if(verb != http::method::unknown)
            return RP->verb_ == verb;
        if(RP->verb_ != http::method::unknown)
            return false;
        return RP->verb_str_ == verb_str;
    }
};

struct router_base::impl
{
    std::vector<entry> entries;
    std::vector<matcher> matchers;

    std::size_t pending_route_ = SIZE_MAX;
    mutable std::once_flag finalized_;

    options_handler_ptr options_handler_;
    std::uint64_t global_methods_ = 0;
    std::vector<std::string> global_custom_verbs_;
    std::string global_allow_header_;

    opt_flags opt_;
    std::size_t depth_ = 0;

    explicit impl(
        opt_flags opt) noexcept
        : opt_(opt)
    {
    }

    void finalize_pending();

    // Thread-safe lazy finalization for dispatch
    void ensure_finalized() const
    {
        std::call_once(finalized_, [this]() {
            const_cast<impl*>(this)->finalize_pending();
        });
    }

    void update_allow_for_entry(
        matcher& m,
        entry const& e);

    void rebuild_global_allow_header();

    route_task
    dispatch_loop(
        route_params& p,
        bool is_options) const;

    static std::string
    build_allow_header(
        std::uint64_t methods,
        std::vector<std::string> const& custom);

    static opt_flags
    compute_effective_opts(
        opt_flags parent,
        opt_flags child);

    static void
    restore_path(
        route_params& p,
        std::size_t base_len);
};

} // detail
} // http
} // boost

#endif
