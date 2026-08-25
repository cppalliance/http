//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZSTD_TYPES_HPP
#define BOOST_HTTP_ZSTD_TYPES_HPP

#include <boost/http/detail/config.hpp>

#include <cstddef>

namespace boost {
namespace http {
namespace zstd {

/** Input buffer for streaming operations.

    Describes a region of input data and tracks how much of
    it has been consumed. Before a call, set @ref src and
    @ref size to the available input and @ref pos to zero.
    The service advances @ref pos as input is consumed; if
    `pos < size` after a call, the remaining input must be
    presented again on the next call.

    @code
    boost::http::zstd::in_buffer in{ data.data(), data.size(), 0 };
    @endcode
*/
struct in_buffer
{
    /** Start of the input buffer. */
    void const* src;

    /** Size of the input buffer in bytes. */
    std::size_t size;

    /** Position where reading stopped.

        Updated by the service; always `0 <= pos <= size`.
    */
    std::size_t pos;
};

/** Output buffer for streaming operations.

    Describes a region of writable memory and tracks how much
    of it has been filled. Before a call, set @ref dst and
    @ref size to the available space and @ref pos to zero.
    The service advances @ref pos as output is produced.

    @code
    boost::http::zstd::out_buffer out{ buf.data(), buf.size(), 0 };
    @endcode
*/
struct out_buffer
{
    /** Start of the output buffer. */
    void* dst;

    /** Size of the output buffer in bytes. */
    std::size_t size;

    /** Position where writing stopped.

        Updated by the service; always `0 <= pos <= size`.
    */
    std::size_t pos;
};

/** Bounds of a compression or decompression parameter.

    Returned by @ref compress_service::param_bounds and
    @ref decompress_service::param_bounds. The @ref error
    field must be tested with the service's `is_error`
    function before the bounds are used.
*/
struct bounds
{
    /** Error status of the query; zero on success. */
    std::size_t error;

    /** Inclusive lower bound of the parameter. */
    int lower_bound;

    /** Inclusive upper bound of the parameter. */
    int upper_bound;
};

/** Context reset directives.

    These values select what is reset when a compression
    or decompression context is reset.
*/
enum class reset_directive
{
    /** Abort the frame in progress.

        Parameters and any loaded dictionary are kept and
        will be used for the next frame. Never fails.
    */
    session_only = 1,

    /** Restore all parameters to their defaults.

        This also drops any dictionary. Fails if a frame
        is in progress.
    */
    parameters = 2,

    /** Reset the session, then the parameters. */
    session_and_parameters = 3
};

/** Frame content size constants.

    Special values returned by
    @ref decompress_service::get_frame_content_size and
    accepted by @ref compress_service::set_pledged_src_size.
*/
enum content_size : unsigned long long
{
    /** The content size is not recorded in the frame header. */
    content_size_unknown = 0ULL - 1,

    /** The frame header is invalid or too short to decode. */
    content_size_error = 0ULL - 2
};

} // zstd
} // http
} // boost

#endif
