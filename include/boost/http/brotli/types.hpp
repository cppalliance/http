//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_BROTLI_TYPES_HPP
#define BOOST_HTTP_BROTLI_TYPES_HPP

#include <boost/http/detail/config.hpp>

#include <cstddef>

namespace boost {
namespace http {
namespace brotli {

/** Allocating function pointer type. */
using alloc_func = void* (*)(void* opaque, std::size_t size);

/** Deallocating function pointer type. */
using free_func = void (*)(void* opaque, void* address);

} // brotli
} // http
} // boost

#endif
