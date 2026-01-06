//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/detail/filter.hpp"

#include <boost/buffers/front.hpp>

namespace boost {
namespace http {
namespace detail {

auto
filter::
process(
    buffers::slice_of<
        boost::span<const buffers::mutable_buffer>> out,
    buffers::const_buffer_pair in,
    bool more) -> results
{
    results rv;
    bool p_more = true;
    for(;;)
    {
        if(!more && p_more && in[1].size() == 0)
        {
            if(buffers::size(out) < min_out_buffer())
            {
                rv.out_short = true;
                return rv;
            }
            p_more = false;
        }

        auto ob = buffers::front(out);
        auto ib = buffers::front(in);
        auto rs = do_process(ob, ib, p_more);

        rv.in_bytes  += rs.in_bytes;
        rv.out_bytes += rs.out_bytes;

        if(rs.ec.failed())
        {
            rv.ec = rs.ec;
            return rv;
        }

        if(rs.finished)
        {
            rv.finished = true;
            return rv;
        }

        buffers::remove_prefix(out, rs.out_bytes);
        buffers::remove_prefix(in, rs.in_bytes);

        if(buffers::size(out) == 0)
            return rv;

        if(buffers::size(in) == 0 && rs.out_bytes < ob.size())
            return rv;
    }
}

} // detail
} // http
} // boost
