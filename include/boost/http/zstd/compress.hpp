//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZSTD_COMPRESS_HPP
#define BOOST_HTTP_ZSTD_COMPRESS_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/zstd/error.hpp>
#include <boost/http/zstd/service.hpp>
#include <boost/http/zstd/types.hpp>

#include <boost/capy/ex/execution_context.hpp>

#include <cstddef>

namespace boost {
namespace http {
namespace zstd {

/** Opaque structure that holds compression context state.

    A context is created with @ref compress_service::create_cctx
    and released with @ref compress_service::free_cctx. It holds
    the sticky parameters and the state of the frame being
    compressed, and may be reused for successive frames.
*/
struct cctx;

/** Opaque structure that holds a digested compression dictionary.

    Created with @ref compress_service::create_cdict and released
    with @ref compress_service::free_cdict. A digested dictionary
    is read-only and may be shared by multiple contexts and threads.
*/
struct cdict;

/** Streaming end directives.

    These values control how @ref compress_service::compress_stream
    treats the input it is given.
*/
enum class end_directive
{
    /** Collect more data; the encoder decides when to emit output. */
    continue_ = 0,

    /** Flush all data provided so far.

        Creates at least one new block that can be decoded
        immediately on reception. The frame continues, so
        future data can still reference previous content.
    */
    flush = 1,

    /** Flush all remaining data and close the current frame. */
    end = 2
};

/** Compression strategies, listed from fastest to strongest.

    Selected with the @ref c_parameter::strategy parameter.
    New strategies may be added in the future; only the
    ordering from fast to strong is guaranteed.
*/
enum class strategy
{
    fast     = 1,
    dfast    = 2,
    greedy   = 3,
    lazy     = 4,
    lazy2    = 5,
    btlazy2  = 6,
    btopt    = 7,
    btultra  = 8,
    btultra2 = 9
};

/** Compression parameter identifiers.

    These values identify parameters that can be set on a
    compression context with @ref compress_service::set_parameter.
    Parameters are sticky: once set they apply to every frame
    compressed with that context until the context's parameters
    are reset. For the bounded parameters, a value of zero
    means "use the default".
*/
enum class c_parameter
{
    /** Compression level; negative values select faster modes. */
    compression_level = 100,

    /** Maximum back-reference distance, as a power of 2.

        This sets the memory budget for streaming decompression,
        with larger values requiring more memory and typically
        compressing better.
    */
    window_log = 101,

    /** Size of the initial probe table, as a power of 2. */
    hash_log = 102,

    /** Size of the multi-probe search table, as a power of 2. */
    chain_log = 103,

    /** Number of search attempts, as a power of 2. */
    search_log = 104,

    /** Minimum size of searched matches. */
    min_match = 105,

    /** Match length considered "good enough" to stop searching. */
    target_length = 106,

    /** Compression strategy, see @ref boost::http::zstd::strategy. */
    strategy = 107,

    /** Enable long distance matching for large inputs. */
    enable_long_distance_matching = 160,

    /** Size of the long distance matching table, as a power of 2. */
    ldm_hash_log = 161,

    /** Minimum match size for the long distance matcher. */
    ldm_min_match = 162,

    /** Log size of each bucket in the long distance matching table. */
    ldm_bucket_size_log = 163,

    /** Frequency of inserting entries into the long distance matching table. */
    ldm_hash_rate_log = 164,

    /** Write the content size into the frame header whenever known (default: 1). */
    content_size_flag = 200,

    /** Write a 32-bit checksum of the content at the end of the frame (default: 0). */
    checksum_flag = 201,

    /** Write the dictionary ID into the frame header when applicable (default: 1). */
    dict_id_flag = 202,

    /** Number of worker threads; zero selects single-threaded mode. */
    nb_workers = 400,

    /** Size of a compression job when using worker threads. */
    job_size = 401,

    /** Overlap between jobs, as a fraction of the window size (0-9). */
    overlap_log = 402
};

/** Provides the Zstandard compression API.

    This service interface exposes Zstandard compression
    functionality through a set of virtual functions. Data
    can be compressed in one shot with @ref compress or
    @ref compress2, or incrementally with @ref compress_stream.

    Most functions return a `std::size_t` which is either a
    byte count or an encoded error code. Test results with
    @ref is_error and convert them with @ref get_error_code
    or @ref get_error_name.

    Compression contexts are reusable: after a frame is
    complete, the same context can compress another frame,
    keeping the parameters that were set on it.

    @code
    // Example: Simple one-shot compression
    auto& compressor = boost::http::zstd::install_compress_service(ctx);

    std::vector<char> input = get_input();
    std::vector<char> output(compressor.compress_bound(input.size()));

    std::size_t n = compressor.compress(
        output.data(), output.size(),
        input.data(), input.size(),
        compressor.default_level());

    if (! compressor.is_error(n))
    {
        output.resize(n);
        // Use compressed data
    }
    @endcode

    @code
    // Example: Streaming compression
    auto* ctx = compressor.create_cctx();

    compressor.set_parameter(ctx,
        boost::http::zstd::c_parameter::compression_level, 5);
    compressor.set_parameter(ctx,
        boost::http::zstd::c_parameter::checksum_flag, 1);

    std::vector<char> buf(compressor.stream_out_size());
    boost::http::zstd::in_buffer in{ input.data(), input.size(), 0 };
    std::size_t remaining;
    do
    {
        boost::http::zstd::out_buffer out{ buf.data(), buf.size(), 0 };
        remaining = compressor.compress_stream(ctx, out, in,
            boost::http::zstd::end_directive::end);
        if (compressor.is_error(remaining))
            break;
        output.insert(output.end(), buf.data(), buf.data() + out.pos);
    }
    while (remaining != 0);

    compressor.free_cctx(ctx);
    @endcode
*/
struct BOOST_SYMBOL_VISIBLE
    compress_service
    : capy::execution_context::service
{
    /** Return the Zstandard library version number.
        @return The version as `MAJOR * 10000 + MINOR * 100 + RELEASE`.
    */
    virtual
    unsigned
    version_number() const noexcept = 0;

    /** Return the Zstandard library version string.
        @return Pointer to a string such as "1.5.7".
    */
    virtual
    char const*
    version_string() const noexcept = 0;

    /** Return the minimum compression level.
        @return The most negative level allowed.
    */
    virtual
    int
    min_level() const noexcept = 0;

    /** Return the maximum compression level.
        @return The highest level available.
    */
    virtual
    int
    max_level() const noexcept = 0;

    /** Return the default compression level.
        @return The level used when none is specified.
    */
    virtual
    int
    default_level() const noexcept = 0;

    /** Return the maximum compressed size in the worst case.
        @param src_size The size of the input data.
        @return An upper bound on the compressed size of a
                single frame, or an error code if `src_size`
                is too large.
    */
    virtual
    std::size_t
    compress_bound(std::size_t src_size) const noexcept = 0;

    /** Compress data in one call as a single frame.
        @param dst Output buffer.
        @param dst_capacity Output buffer size.
        @param src Input data.
        @param src_size Input data size.
        @param level The compression level.
        @return The compressed size, or an error code.
    */
    virtual
    std::size_t
    compress(
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t src_size,
        int level) const noexcept = 0;

    /** Create a new compression context.
        @return Pointer to the context, or nullptr on error.
    */
    virtual
    cctx*
    create_cctx() const noexcept = 0;

    /** Release a compression context.
        @param ctx The context to release; may be nullptr.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    free_cctx(cctx* ctx) const noexcept = 0;

    /** Return the current memory usage of a compression context.
        @param ctx The context.
        @return Memory usage in bytes.
    */
    virtual
    std::size_t
    sizeof_cctx(cctx const* ctx) const noexcept = 0;

    /** Return the valid bounds of a compression parameter.
        @param param The parameter identifier.
        @return The bounds; test the `error` field with @ref is_error.
    */
    virtual
    bounds
    param_bounds(c_parameter param) const noexcept = 0;

    /** Set a compression parameter.

        Parameters can only be set between frames, before
        compression of the next frame starts. Values beyond
        the bounds are either clamped or rejected, depending
        on the parameter.

        @param ctx The context.
        @param param The parameter identifier.
        @param value The parameter value.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    set_parameter(
        cctx* ctx,
        c_parameter param,
        int value) const noexcept = 0;

    /** Declare the total input size of the next frame.

        The value is written into the frame header and checked
        at the end of the frame. It applies to the next frame
        only; afterwards the size reverts to unknown.

        @param ctx The context.
        @param pledged_src_size The input size, or
               @ref content_size_unknown.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    set_pledged_src_size(
        cctx* ctx,
        unsigned long long pledged_src_size) const noexcept = 0;

    /** Reset a compression context.
        @param ctx The context.
        @param directive What to reset.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    reset(
        cctx* ctx,
        reset_directive directive) const noexcept = 0;

    /** Compress data in one call using a context's parameters.

        Always starts a new frame; any unfinished frame held
        by the context is discarded.

        @param ctx The context.
        @param dst Output buffer.
        @param dst_capacity Output buffer size.
        @param src Input data.
        @param src_size Input data size.
        @return The compressed size, or an error code.
    */
    virtual
    std::size_t
    compress2(
        cctx* ctx,
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t src_size) const noexcept = 0;

    /** Compress data in streaming mode.

        Consumes input from `input` and writes output to
        `output`, advancing the `pos` field of each. Input
        may not be fully consumed if the output buffer fills
        up; present the remaining input again after making
        room for more output.

        @param ctx The context.
        @param output The output buffer.
        @param input The input buffer.
        @param end_op The end directive.
        @return A minimum estimate of the bytes still buffered
                internally, or an error code. With
                @ref end_directive::flush or @ref end_directive::end,
                keep calling with the same directive until zero
                is returned.
    */
    virtual
    std::size_t
    compress_stream(
        cctx* ctx,
        out_buffer& output,
        in_buffer& input,
        end_directive end_op) const noexcept = 0;

    /** Return the recommended input buffer size for streaming.
        @return Size in bytes.
    */
    virtual
    std::size_t
    stream_in_size() const noexcept = 0;

    /** Return the recommended output buffer size for streaming.

        An output buffer of this size is guaranteed to be able
        to flush at least one complete compressed block.

        @return Size in bytes.
    */
    virtual
    std::size_t
    stream_out_size() const noexcept = 0;

    /** Create a digested dictionary for compression.
        @param dict The dictionary content; copied internally.
        @param dict_size The dictionary size.
        @param level The compression level to digest for.
        @return Pointer to the dictionary, or nullptr on error.
    */
    virtual
    cdict*
    create_cdict(
        void const* dict,
        std::size_t dict_size,
        int level) const noexcept = 0;

    /** Release a digested dictionary.
        @param dict The dictionary to release; may be nullptr.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    free_cdict(cdict* dict) const noexcept = 0;

    /** Return the current memory usage of a digested dictionary.
        @param dict The dictionary.
        @return Memory usage in bytes.
    */
    virtual
    std::size_t
    sizeof_cdict(cdict const* dict) const noexcept = 0;

    /** Load a dictionary into a context.

        The content is copied and digested; it is used for all
        future frames until the parameters are reset or another
        dictionary is loaded. Loading a null or empty dictionary
        returns to no-dictionary mode.

        @param ctx The context.
        @param dict The dictionary content.
        @param dict_size The dictionary size.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    load_dictionary(
        cctx* ctx,
        void const* dict,
        std::size_t dict_size) const noexcept = 0;

    /** Reference a digested dictionary from a context.

        The dictionary is only referenced and must outlive its
        use by the context. Its compression parameters supersede
        those set on the context. Referencing nullptr returns
        to no-dictionary mode.

        @param ctx The context.
        @param dict The digested dictionary.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    ref_cdict(
        cctx* ctx,
        cdict const* dict) const noexcept = 0;

    /** Reference a prefix for the next frame.

        A prefix is a single-use dictionary of raw content,
        discarded once the frame ends. The buffer is only
        referenced and must remain valid and unmodified while
        the frame is compressed.

        @param ctx The context.
        @param prefix The prefix content.
        @param prefix_size The prefix size.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    ref_prefix(
        cctx* ctx,
        void const* prefix,
        std::size_t prefix_size) const noexcept = 0;

    /** Return the dictionary ID stored within a dictionary.
        @param dict The dictionary content.
        @param dict_size The dictionary size.
        @return The dictionary ID, or zero if the content is
                not a conformant dictionary.
    */
    virtual
    unsigned
    get_dict_id_from_dict(
        void const* dict,
        std::size_t dict_size) const noexcept = 0;

    /** Check whether a result is an error code.
        @param result A value returned from a function of this service.
        @return True if the result encodes an error.
    */
    virtual
    bool
    is_error(std::size_t result) const noexcept = 0;

    /** Convert a result to an error code.
        @param result A value returned from a function of this service.
        @return The error code, or @ref error::no_error.
    */
    virtual
    error
    get_error_code(std::size_t result) const noexcept = 0;

    /** Return a readable description of a result.
        @param result A value returned from a function of this service.
        @return Pointer to a description string.
    */
    virtual
    char const*
    get_error_name(std::size_t result) const noexcept = 0;

    /** Return a string description of an error code.
        @param c The error code.
        @return Pointer to error description string.
    */
    virtual
    char const*
    error_string(error c) const noexcept = 0;

protected:
    void shutdown() override {}
};

} // zstd
} // http
} // boost

#endif
