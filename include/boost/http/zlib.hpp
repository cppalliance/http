//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

/** @file
    ZLib compression and decompression library.

    This header includes all ZLib-related functionality including
    deflate, inflate, error handling, and stream management.

    ZLib is a widely-used lossless data compression library that
    implements the DEFLATE compression algorithm. It provides both
    compression (deflate) and decompression (inflate) functionality
    with support for raw, zlib, and gzip formats through the
    windowBits parameter.

    @code
    #include <boost/http/zlib.hpp>
    #include <boost/capy/datastore.hpp>

    // Create a datastore for services
    boost::capy::datastore ctx;

    // Install compression and decompression services
    auto& deflate_svc = boost::http::zlib::install_deflate_service(ctx);
    auto& inflate_svc = boost::http::zlib::install_inflate_service(ctx);
    @endcode
*/

#ifndef BOOST_HTTP_ZLIB_HPP
#define BOOST_HTTP_ZLIB_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/zlib/compression_level.hpp>
#include <boost/http/zlib/compression_method.hpp>
#include <boost/http/zlib/compression_strategy.hpp>
#include <boost/http/zlib/data_type.hpp>
#include <boost/http/zlib/deflate.hpp>
#include <boost/http/zlib/error.hpp>
#include <boost/http/zlib/flush.hpp>
#include <boost/http/zlib/inflate.hpp>
#include <boost/http/zlib/stream.hpp>

#endif
