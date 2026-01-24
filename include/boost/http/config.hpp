//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_CONFIG_HPP
#define BOOST_HTTP_CONFIG_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/header_limits.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace boost {
namespace http {

/** Parser configuration settings.

    @see @ref make_parser_config,
         @ref request_parser,
         @ref response_parser.
*/
struct parser_config
{
    /// Limits for HTTP headers.
    header_limits headers;

    /** Maximum content body size (after decoding).

        @see @ref parser::set_body_limit.
    */
    std::uint64_t body_limit;

    /** Enable Brotli Content-Encoding decoding.
    */
    bool apply_brotli_decoder = false;

    /** Enable Deflate Content-Encoding decoding.
    */
    bool apply_deflate_decoder = false;

    /** Enable Gzip Content-Encoding decoding.
    */
    bool apply_gzip_decoder = false;

    /** Zlib window bits (9-15).

        Must be >= the value used during compression.
        Larger windows improve decompression at the
        cost of memory.
    */
    int zlib_window_bits = 15;

    /** Minimum payload buffer size.

        Controls:
        @li Smallest read/decode buffer allocation
        @li Minimum guaranteed in-place body size
        @li Reserve size for dynamic buffers when
            payload size is unknown

        This cannot be zero.
    */
    std::size_t min_buffer = 4096;

    /** Maximum buffer size from @ref parser::prepare.

        This cannot be zero.
    */
    std::size_t max_prepare = std::size_t(-1);

    /** Space reserved for type-erased @ref sink objects.
    */
    std::size_t max_type_erase = 1024;

    /** Constructor.

        @param server True for server mode (parsing requests,
               64KB body limit), false for client mode
               (parsing responses, 1MB body limit).
    */
    explicit
    parser_config(bool server) noexcept
        : body_limit(server ? 64 * 1024 : 1024 * 1024)
    {
    }
};

/** Parser configuration with computed fields.

    Derived from @ref parser_config with additional
    precomputed values for workspace allocation.

    @see @ref make_parser_config.
*/
struct parser_config_impl : parser_config
{
    /// Total workspace allocation size.
    std::size_t space_needed;

    /// Space for decompressor state.
    std::size_t max_codec;

    /// Maximum overread bytes.
    BOOST_HTTP_DECL
    std::size_t
    max_overread() const noexcept;
};

//------------------------------------------------

/** Serializer configuration settings.

    @see @ref make_serializer_config,
         @ref serializer.
*/
struct serializer_config
{
    /** Enable Brotli Content-Encoding.
    */
    bool apply_brotli_encoder = false;

    /** Enable Deflate Content-Encoding.
    */
    bool apply_deflate_encoder = false;

    /** Enable Gzip Content-Encoding.
    */
    bool apply_gzip_encoder = false;

    /** Brotli compression quality (0-11).

        Higher values yield better but slower compression.
    */
    std::uint32_t brotli_comp_quality = 5;

    /** Brotli compression window size (10-24).

        Larger windows improve compression but increase
        memory usage.
    */
    std::uint32_t brotli_comp_window = 18;

    /** Zlib compression level (0-9).

        0 = no compression, 1 = fastest, 9 = best.
    */
    int zlib_comp_level = 6;

    /** Zlib window bits (9-15).

        Controls the history buffer size.
    */
    int zlib_window_bits = 15;

    /** Zlib memory level (1-9).

        Higher values use more memory, but offer faster
        and more efficient compression.
    */
    int zlib_mem_level = 8;

    /** Minimum buffer size for payloads (must be > 0).
    */
    std::size_t payload_buffer = 8192;

    /** Reserved space for type-erasure storage.
    */
    std::size_t max_type_erase = 1024;
};

//------------------------------------------------

struct parser_config_impl;
struct serializer_config_impl;

/** Shared pointer to immutable parser configuration.

    @see @ref parser_config_impl, @ref make_parser_config.
*/
using shared_parser_config = std::shared_ptr<parser_config_impl const>;

/** Shared pointer to immutable serializer configuration.

    @see @ref serializer_config_impl, @ref make_serializer_config.
*/
using shared_serializer_config = std::shared_ptr<serializer_config_impl const>;


/** Create parser configuration with computed values.

    @param cfg User-provided configuration settings.

    @return Shared pointer to configuration with
            precomputed fields.

    @see @ref parser_config,
         @ref request_parser,
         @ref response_parser.
*/
BOOST_HTTP_DECL
shared_parser_config
make_parser_config(parser_config cfg);

/** Serializer configuration with computed fields.

    Derived from @ref serializer_config with additional
    precomputed values for workspace allocation.

    @see @ref make_serializer_config.
*/
struct serializer_config_impl : serializer_config
{
    /// Total workspace allocation size.
    std::size_t space_needed;
};

/** Create serializer configuration with computed values.

    @param cfg User-provided configuration settings.

    @return Shared pointer to configuration with
            precomputed fields.

    @see @ref serializer_config,
         @ref serializer.
*/
BOOST_HTTP_DECL
shared_serializer_config
make_serializer_config(serializer_config cfg);

} // http
} // boost

#endif
