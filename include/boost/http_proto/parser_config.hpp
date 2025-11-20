//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_PARSERCONFIG_HPP
#define BOOST_HTTP_PROTO_PARSERCONFIG_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/header_limits.hpp>
#include <boost/capy/polystore.hpp>
#include <cstdint>
#include <memory>

namespace boost {

// VFALCO these should go in capy/zlib_fwd and capy/brotli_fwd
namespace capy {
namespace brotli {
struct decode_service;
struct encode_service;
} // brotli
namespace zlib {
struct deflate_service;
struct inflate_service;
} // zlib
} // capy

namespace http_proto {

class parser;

enum class role
{
    client,
    server
};

struct prepared_parser_config;

//-----------------------------------------------

/** Parser configuration settings.
*/
struct parser_config
{
    /** Configurable limits for HTTP headers.
    */
    header_limits headers;

    /** Maximum allowed size of the content body.

        Measured after decoding.
    */
    std::uint64_t body_limit = 64 * 1024;

    /** Enable Brotli Content-Encoding decoding.

        Requires `boost::capy::brotli::decode_service` to be
        installed, otherwise an exception is thrown.
    */
    bool apply_brotli_decoder = false;

    /** Enable Deflate Content-Encoding decoding.

        Requires `boost::zlib::inflate_service` to be
        installed, otherwise an exception is thrown.
    */
    bool apply_deflate_decoder = false;

    /** Enable Gzip Content-Encoding decoding.

        Requires `boost::zlib::inflate_service` to be
        installed, otherwise an exception is thrown.
    */
    bool apply_gzip_decoder = false;

    /** Zlib window bits (9–15).

        Must be >= the value used during compression.
        Larger windows improve decompression at the cost
        of memory. If a larger window is required than
        allowed, decoding fails with
        `capy::zlib::error::data_err`.
    */
    int zlib_window_bits = 15;

    /** Minimum space for payload buffering.

        This value controls the following
        settings:

        @li The smallest allocated size of
            the buffers used for reading
            and decoding the payload.

        @li The lowest guaranteed size of
            an in-place body.

        @li The largest size used to reserve
            space in dynamic buffer bodies
            when the payload size is not
            known ahead of time.

        This cannot be zero.
    */
    std::size_t min_buffer = 4096;

    /** Largest permissible output size in prepare.

        This cannot be zero.
    */
    std::size_t max_prepare = std::size_t(-1);

    /** Space to reserve for type-erasure.

        This space is used for the following
        purposes:

        @li Storing an instance of the user-provided
            @ref sink objects.

        @li Storing an instance of the user-provided
            ElasticBuffer.
    */
    std::size_t max_type_erase = 1024;

    /** Constructor.
        @param role Indicates whether the parser
        will be used for client or server messages.
        @param ps The polystore to obtain services from.
    */
    BOOST_HTTP_PROTO_DECL
    parser_config(role role, capy::polystore& ps);

    auto prepare() const ->
        prepared_parser_config;

private:
    friend class parser;
    BOOST_HTTP_PROTO_DECL static void prepare(
        std::shared_ptr<parser_config> sp);

    // VFALCO remove this, its ugly here
    std::size_t max_overread() const noexcept
    {
        return
            headers.max_size +
            min_buffer;
    }

    capy::polystore& ps_;
    capy::brotli::decode_service* brotli_decode = nullptr;
    capy::brotli::encode_service* brotli_encode = nullptr;
    capy::zlib::inflate_service* zlib_deflate = nullptr;
    capy::zlib::inflate_service* zlib_inflate = nullptr;
    std::size_t space_needed = 0;
    std::size_t max_codec = 0; 
};

//-----------------------------------------------

struct prepared_parser_config
{
    parser_config const*
    operator->() const noexcept
    {
        return sp_.get();
    }

private:
    friend class parser;
    friend struct parser_config;

// VFALCO REMOVE THIS
prepared_parser_config() = default;

    prepared_parser_config(
        std::shared_ptr<parser_config const> sp)
         : sp_(std::move(sp))
    {
    }

    std::shared_ptr<parser_config const> sp_;
};

//-----------------------------------------------

inline
auto
parser_config::
prepare() const ->
    prepared_parser_config
{
    auto sp = std::make_shared<
        parser_config>(std::move(*this));
    prepare(sp);
    return prepared_parser_config(std::move(sp));
}

} // http_proto
} // boost

#endif
