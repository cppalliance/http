//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_BUFFER_UTILS_HPP
#define BOOST_HTTP_DETAIL_BUFFER_UTILS_HPP

#include <boost/capy/buffers.hpp>
#include <boost/core/span.hpp>
#include <iterator>

namespace boost {
namespace http {
namespace detail {

template<typename BufferSequence>
auto
make_span(BufferSequence const& bs) ->
    boost::span<typename std::iterator_traits<
        decltype(capy::buffer_sequence_begin(bs))>::value_type const>
{
    return { &*capy::buffer_sequence_begin(bs),
        std::size_t(std::distance(
            capy::buffer_sequence_begin(bs), capy::buffer_sequence_end(bs))) };
}

} // detail
} // http
} // boost

#endif
