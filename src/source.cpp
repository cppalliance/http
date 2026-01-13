//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/source.hpp>

namespace boost {
namespace http {

auto
source::
on_read(
    boost::span<capy::mutable_buffer const> bs) ->
        results
{
    results rv;
    auto it = bs.begin();
    auto const end_ = bs.end();
    if(it == end_)
        return rv;
    do
    {
        capy::mutable_buffer b(*it++);
        auto rs = on_read(b);
        rv += rs;
        if(rs.ec.failed())
            return rv;
        if(rs.finished)
            break;
        // source must fill the entire buffer
        // if it is not finished
        if(b.size() != rs.bytes)
            detail::throw_logic_error();
    }
    while(it != end_);
    return rv;
}

} // http
} // boost
