//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

/** @file
    Brotli compression and decompression library.

    This header includes all Brotli-related functionality including
    encoding, decoding, error handling, and shared dictionary support.

    Brotli is a generic-purpose lossless compression algorithm that compresses
    data using a combination of a modern variant of the LZ77 algorithm, Huffman
    coding and 2nd order context modeling, with a compression ratio comparable
    to the best currently available general-purpose compression methods.

    @code
    #include <boost/http/brotli.hpp>
    #include <boost/http/datastore.hpp>

    // Create a datastore for services
    boost::http::datastore ctx;

    // Install compression and decompression services
    auto& encoder = boost::http::brotli::install_encode_service(ctx);
    auto& decoder = boost::http::brotli::install_decode_service(ctx);
    @endcode
*/

#ifndef BOOST_HTTP_BROTLI_HPP
#define BOOST_HTTP_BROTLI_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/brotli/decode.hpp>
#include <boost/http/brotli/encode.hpp>
#include <boost/http/brotli/error.hpp>
#include <boost/http/brotli/shared_dictionary.hpp>
#include <boost/http/brotli/types.hpp>

#endif
