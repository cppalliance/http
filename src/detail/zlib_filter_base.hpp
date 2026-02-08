//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/buffers
//

#ifndef BOOST_HTTP_DETAIL_ZLIB_FILTER_BASE_HPP
#define BOOST_HTTP_DETAIL_ZLIB_FILTER_BASE_HPP

#include "src/detail/filter.hpp"

#include <boost/http/zlib/stream.hpp>
#include <boost/http/zlib/flush.hpp>

namespace boost {
namespace http {
namespace detail {

/** Base class for zlib filters
*/
class zlib_filter_base : public filter
{
public:
    zlib_filter_base()
    {
        strm_.zalloc = nullptr;
        strm_.zfree  = nullptr;
        strm_.opaque = nullptr;
    }

protected:
    http::zlib::stream strm_;

    static
    unsigned int
    saturate_cast(std::size_t n) noexcept
    {
        if(n >= std::numeric_limits<unsigned int>::max())
            return std::numeric_limits<unsigned int>::max();
        return static_cast<unsigned int>(n);
    }
};

} // detail
} // http
} // boost

#endif
