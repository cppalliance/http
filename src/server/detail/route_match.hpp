//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_DETAIL_ROUTE_MATCH_HPP
#define BOOST_HTTP_SERVER_DETAIL_ROUTE_MATCH_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/any_router.hpp>
#include "src/server/route_abnf.hpp"
#include "src/server/detail/stable_string.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace boost {
namespace http {

// Matches a path against a pattern
// Members ordered largest-to-smallest for optimal packing
struct any_router::matcher
{
    matcher(std::string_view pat, bool end_);

    // true if match
    bool operator()(
        route_params& p,
        match_result& mr) const;

    // Returns error from pattern parsing, or empty if valid
    system::error_code error() const noexcept { return ec_; }

private:
    friend class any_router;
    friend struct any_router::impl;

    system::error_code ec_;
    std::string allow_header_;
    detail::route_pattern pattern_;
    std::vector<std::string> custom_verbs_;

    // 16 bytes (pointer + size)
    detail::stable_string decoded_pat_;

    // 8 bytes each
    std::size_t first_entry_ = 0;
    std::size_t skip_ = 0;
    std::uint64_t allowed_methods_ = 0;

    // 4 bytes each
    opt_flags effective_opts_ = 0;
    opt_flags own_opts_ = 0;        // router's opt_flags (for re-parenting during inline)
    std::uint32_t depth_ = 0;

    // 1 byte each
    bool end_;      // false for middleware
    bool slash_;
};

} // http
} // boost

#endif
