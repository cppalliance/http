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
#include <boost/http/server/basic_router.hpp>
#include "src/server/detail/route_rule.hpp"
#include "src/server/detail/stable_string.hpp"

namespace boost {
namespace http {
namespace detail {

// Matches a path against a pattern
// Members ordered largest-to-smallest for optimal packing
struct router_base::matcher
{
    friend class http::flat_router;

    matcher(std::string_view pat, bool end_);

    // true if match
    bool operator()(
        route_params_base& p,
        match_result& mr) const;

private:
    // 24 bytes (vector)
    path_rule_t::value_type pv_;

    // 16 bytes (pointer + size)
    stable_string decoded_pat_;

    // 8 bytes each
    std::size_t first_entry_ = 0;   // flat_router: first entry using this matcher
    std::size_t skip_ = 0;          // flat_router: entry index to jump to on failure

    // 4 bytes each
    opt_flags effective_opts_ = 0;  // flat_router: computed opts for this scope
    std::uint32_t depth_ = 0;       // flat_router: nesting level (0 = root)

    // 1 byte each
    bool end_;      // false for middleware
    bool slash_;
};

} // detail
} // http
} // boost

#endif
