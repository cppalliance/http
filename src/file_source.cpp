//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/file_source.hpp>

namespace boost {
namespace http {

file_source::
~file_source() = default;

file_source::
file_source(file_source&&) noexcept = default;

file_source::
file_source(
    capy::file&& f,
    std::uint64_t limit) noexcept
    : f_(std::move(f))
    , n_(limit)
{
}

auto
file_source::
on_read(
    capy::mutable_buffer b) -> results
{
    results rv;
    if(n_ > 0)
    {
        std::size_t n;
        if( n_ >= b.size())
            n = b.size();
        else
            n = static_cast<std::size_t>(n_);
        n = f_.read(
            b.data(), n, rv.ec);
        rv.bytes = n;
        if(n == 0 && b.size() != 0 && !rv.ec)
        {
            rv.finished = true;
            return rv;
        }
        n_ -= n;
    }
    rv.finished = n_ == 0;
    return rv;
}

} // http
} // boost
