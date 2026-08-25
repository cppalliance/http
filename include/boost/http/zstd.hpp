//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

/** @file
    Zstandard compression and decompression library.

    This header includes all Zstandard-related functionality including
    compression, decompression, dictionary support, and error handling.

    Zstandard (zstd) is a fast lossless compression algorithm targeting
    real-time compression scenarios. It offers a very wide range of
    compression / speed trade-offs through its compression levels,
    compresses at zlib-like speeds with better ratios, and is backed by
    a very fast decoder whose speed does not depend on the level used.

    @code
    #include <boost/http/zstd.hpp>
    #include <boost/http/datastore.hpp>

    // Create a datastore for services
    boost::http::datastore ctx;

    // Install compression and decompression services
    auto& compressor = boost::http::zstd::install_compress_service(ctx);
    auto& decompressor = boost::http::zstd::install_decompress_service(ctx);
    @endcode
*/

#ifndef BOOST_HTTP_ZSTD_HPP
#define BOOST_HTTP_ZSTD_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/zstd/compress.hpp>
#include <boost/http/zstd/decompress.hpp>
#include <boost/http/zstd/error.hpp>
#include <boost/http/zstd/service.hpp>
#include <boost/http/zstd/types.hpp>

#endif
