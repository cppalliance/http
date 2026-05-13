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

#include <boost/capy/buffers/buffer_slice.hpp>
#include <boost/capy/buffers/front.hpp>

namespace boost {
namespace http {
namespace detail {

namespace {

// Returns true if the slice's current data view contains at most one
// non-empty buffer (i.e., we are processing the last logical chunk).
template<class Slice>
bool single_or_empty(Slice const& s)
{
    auto d = s.data();
    auto it = d.begin();
    auto const end_it = d.end();
    if(it == end_it)
        return true;
    ++it;
    return it == end_it;
}

} // anonymous

auto
filter::
process(
    boost::span<const capy::mutable_buffer> out_seq,
    std::array<capy::const_buffer, 2> in_seq,
    bool more) -> results
{
    auto out = capy::buffer_slice(out_seq);
    auto in = capy::buffer_slice(in_seq);

    results rv;
    bool p_more = true;
    for(;;)
    {
        if(!more && p_more && single_or_empty(in))
        {
            if(capy::buffer_size(out.data()) < min_out_buffer())
            {
                rv.out_short = true;
                return rv;
            }
            p_more = false;
        }

        auto ob = capy::front(out.data());
        auto ib = capy::front(in.data());
        auto rs = do_process(ob, ib, p_more);

        rv.in_bytes  += rs.in_bytes;
        rv.out_bytes += rs.out_bytes;

        if(rs.ec)
        {
            rv.ec = rs.ec;
            return rv;
        }

        if(rs.finished)
        {
            rv.finished = true;
            return rv;
        }

        out.remove_prefix(rs.out_bytes);
        in.remove_prefix(rs.in_bytes);

        if(capy::buffer_size(out.data()) == 0)
            return rv;

        if(capy::buffer_size(in.data()) == 0 && rs.out_bytes < ob.size())
            return rv;
    }
}

} // detail
} // http
} // boost
