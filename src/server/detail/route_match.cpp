//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/server/detail/pct_decode.hpp"
#include "src/server/detail/route_match.hpp"

namespace boost {
namespace http {

any_router::
matcher::
matcher(
    std::string_view pat,
    bool end_arg)
    : decoded_pat_(
        [&pat]
        {
            auto s = detail::pct_decode(pat);
            if( s.size() > 1
                && s.back() == '/')
                s.pop_back();
            return s;
        }())
    , end_(end_arg)
    , slash_(pat == "/")
{
    if(! slash_)
    {
        auto rv = parse_route_pattern(decoded_pat_);
        if(rv.has_error())
            ec_ = rv.error();
        else
            pattern_ = std::move(rv.value());
    }
}

bool
any_router::
matcher::
operator()(
    route_params_base& p,
    match_result& mr) const
{
    BOOST_ASSERT(! p.path.empty());

    // Root pattern special case
    if(slash_ && (!end_ || p.path == "/"))
    {
        mr.adjust_path(p, 0);
        return true;
    }

    // Convert bitflags to match_options
    detail::match_options opts{
        p.case_sensitive,
        p.strict,
        end_
    };

    auto rv = match_route(p.path, pattern_, opts);
    if(rv.has_error())
        return false;

    auto const n = rv->matched_length;
    mr.adjust_path(p, n);
    mr.params_ = std::move(rv->params);
    return true;
}

} // http
} // boost
