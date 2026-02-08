//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZLIB_COMPRESSION_STRATEGY_HPP
#define BOOST_HTTP_ZLIB_COMPRESSION_STRATEGY_HPP

#include <boost/http/detail/config.hpp>

namespace boost {
namespace http {
namespace zlib {

/** Compression strategy constants.

    These values tune the compression algorithm for specific
    types of input data. The default strategy works well for
    most data. Other strategies optimize for specific patterns:
    filtered for data with small value differences, huffman_only
    for pre-compressed data, rle for image data with many
    repeated bytes, and fixed for fastest compression.

    @code
    // Example: Using different strategies for different data types
    boost::http::zlib::stream st = {};

    // For PNG image data (many repeated bytes)
    deflate_svc.init2(st, 6, boost::http::zlib::deflated,
        15, 8, boost::http::zlib::rle);

    // For small numeric differences (filtered data)
    deflate_svc.init2(st, 6, boost::http::zlib::deflated,
        15, 8, boost::http::zlib::filtered);

    // For fastest speed with pre-compressed data
    deflate_svc.init2(st, 1, boost::http::zlib::deflated,
        15, 8, boost::http::zlib::huffman_only);
    @endcode
*/
enum compression_strategy
{
    /** Use the default compression strategy. */
    default_strategy = 0,

    /** Strategy optimized for data with small values. */
    filtered         = 1,

    /** Force Huffman encoding only (no string match). */
    huffman_only     = 2,

    /** Limit match distances to one (run-length encoding). */
    rle              = 3,

    /** Prevent use of dynamic Huffman codes. */
    fixed            = 4
};

} // zlib
} // http
} // boost

#endif
