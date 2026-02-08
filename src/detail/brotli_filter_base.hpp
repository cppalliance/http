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

#include "src/detail/filter.hpp"

namespace boost {
namespace http {
namespace detail {

/** Base class for brotli filters
*/
class brotli_filter_base : public filter
{
};

} // detail
} // http
} // boost

#endif
