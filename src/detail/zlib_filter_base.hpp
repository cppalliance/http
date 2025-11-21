//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/buffers
//

#ifndef BOOST_HTTP_PROTO_DETAIL_ZLIB_FILTER_BASE_HPP
#define BOOST_HTTP_PROTO_DETAIL_ZLIB_FILTER_BASE_HPP

#include <boost/http_proto/detail/workspace.hpp>

#include "src/detail/filter.hpp"

#include <boost/capy/zlib/stream.hpp>
#include <boost/capy/zlib/flush.hpp>

namespace boost {
namespace http_proto {
namespace detail {

/** Base class for zlib filters
*/
class zlib_filter_base : public filter
{
public:
    zlib_filter_base(workspace& w)
    {
        strm_.zalloc = &zalloc;
        strm_.zfree  = &zfree;
        strm_.opaque = &w;
    }

protected:
    capy::zlib::stream strm_;

    static
    unsigned int
    saturate_cast(std::size_t n) noexcept
    {
        if(n >= std::numeric_limits<unsigned int>::max())
            return std::numeric_limits<unsigned int>::max();
        return static_cast<unsigned int>(n);
    }

private:
    static
    void*
    zalloc(
        void* opaque,
        unsigned items,
        unsigned size) noexcept
    {
        return reinterpret_cast<workspace*>(opaque)
            ->try_reserve_front(items * size);
    }

    static
    void
    zfree(
        void* /* opaque */,
        void* /* addr */) noexcept
    {
        // no-op
    }
};

} // detail
} // http_proto
} // boost

#endif
