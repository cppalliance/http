//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/buffers
//

#ifndef BOOST_HTTP_DETAIL_BROTLI_FILTER_BASE_HPP
#define BOOST_HTTP_DETAIL_BROTLI_FILTER_BASE_HPP

#include <boost/http/detail/workspace.hpp>

#include "src/detail/filter.hpp"

namespace boost {
namespace http {
namespace detail {

/** Base class for brotli filters
*/
class brotli_filter_base : public filter
{
protected:
    static
    void*
    alloc(void* opaque, std::size_t size) noexcept
    {
        return reinterpret_cast<detail::workspace*>(opaque)
            ->try_reserve_front(size);
    }

    static
    void
    free(
        void* /* opaque */,
        void* /* addr */) noexcept
    {
        // no-op
    }
};

} // detail
} // http
} // boost

#endif
