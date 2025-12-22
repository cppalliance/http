//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/file_sink.hpp>

namespace boost {
namespace http_proto {

file_sink::
file_sink(
    capy::file&& f) noexcept
    : f_(std::move(f))
{
}

file_sink::
file_sink(file_sink&&) noexcept = default;

file_sink::
~file_sink() = default;

auto
file_sink::
on_write(
    buffers::const_buffer b,
    bool more) -> results
{
    results rv;
    rv.bytes = f_.write(
        b.data(), b.size(), rv.ec);
    if(!more && !rv.ec)
        f_.close(rv.ec);
    return rv;
}

} // http_proto
} // boost
