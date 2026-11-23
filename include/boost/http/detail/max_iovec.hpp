//
// Copyright (c) 2026 Steve Gerbino
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_MAX_IOVEC_HPP
#define BOOST_HTTP_DETAIL_MAX_IOVEC_HPP

#include <cstddef>

namespace boost {
namespace http {
namespace detail {

// Buffers the sources, sinks and transfer loops hand on per call.
// POSIX guarantees an IOV_MAX of at least 16, so one window fits one
// vectored call on any backend. A larger window copies descriptors a
// backend may not use in that call, and each one adds 16 bytes to the
// awaiting frame.
inline constexpr std::size_t max_iovec_ = 16;

} // detail
} // http
} // boost

#endif
