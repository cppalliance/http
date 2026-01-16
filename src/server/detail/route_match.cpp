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
namespace detail {

router_base::
matcher::
matcher(
    std::string_view pat,
    bool end_arg)
    : decoded_pat_(
        [&pat]
        {
            auto s = pct_decode(pat);
            if( s.size() > 1
                && s.back() == '/')
                s.pop_back();
            return s;
        }())
    , end_(end_arg)
    , slash_(pat == "/")
{
    if(! slash_)
        pv_ = grammar::parse(
            decoded_pat_, detail::path_rule).value();
}

bool
router_base::
matcher::
operator()(
    route_params_base& p,
    match_result& mr) const
{
    BOOST_ASSERT(! p.path.empty());
    if( slash_ && (
        ! end_ ||
        p.path == "/"))
    {
        // params = {};
        mr.adjust_path(p, 0);
        return true;
    }
    auto it = p.path.data();
    auto pit = pv_.segs.begin();
    auto const path_end = it + p.path.size();
    auto const pend = pv_.segs.end();
    while(it != path_end && pit != pend)
    {
        // prefix has to match
        auto s = core::string_view(it, path_end);
        if(! p.case_sensitive)
        {
            if(pit->prefix.size() > s.size())
                return false;
            s = s.substr(0, pit->prefix.size());
            //if(! grammar::ci_is_equal(s, pit->prefix))
            if(! ci_is_equal(s, pit->prefix))
                return false;
        }
        else
        {
            if(! s.starts_with(pit->prefix))
                return false;
        }
        it += pit->prefix.size();
        ++pit;
    }
    if(end_)
    {
        // require full match
        if( it != path_end ||
            pit != pend)
            return false;
    }
    else if(pit != pend)
    {
        return false;
    }
    // number of matching characters
    auto const n = it - p.path.data();
    mr.adjust_path(p, n);
    return true;
}

} // detail
} // http
} // boost
