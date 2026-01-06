//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_IMPL_SINK_HPP
#define BOOST_HTTP_IMPL_SINK_HPP

#include <boost/buffers/buffer.hpp>
#include <boost/buffers/range.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace http {

inline
auto
sink::
results::
operator+=(
    results const& rv) noexcept ->
        results&
{
    BOOST_ASSERT(! ec.failed());
    ec = rv.ec;
    bytes += rv.bytes;
    return *this;
}

//------------------------------------------------

template<class T>
auto
sink::
write_impl(
    T const& bs,
    bool more) ->
        results
{
    results rv;
    constexpr int SmallArraySize = 16;
    buffers::const_buffer tmp[SmallArraySize];
    auto const tmp_end = tmp + SmallArraySize;
    auto it = buffers::begin(bs);
    auto const end_ = buffers::end(bs);
    while(it != end_)
    {
        auto p = tmp;
        do
        {
            *p++ = *it++;
        }
        while(
            p != tmp_end &&
            it != end_);
        rv += on_write(boost::span<
            buffers::const_buffer const>(
                tmp, p - tmp),
            it != end_ || more);
        if(rv.ec.failed())
            return rv;
    }
    return rv;
}

} // http
} // boost

#endif
