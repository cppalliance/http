//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZSTD_DECOMPRESS_HPP
#define BOOST_HTTP_ZSTD_DECOMPRESS_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/zstd/error.hpp>
#include <boost/http/zstd/service.hpp>
#include <boost/http/zstd/types.hpp>

#include <boost/capy/ex/execution_context.hpp>

#include <cstddef>

namespace boost {
namespace http {
namespace zstd {

/** Opaque structure that holds decompression context state.

    A context is created with @ref decompress_service::create_dctx
    and released with @ref decompress_service::free_dctx. It holds
    the sticky parameters and the state of the frame being
    decompressed, and may be reused for successive frames.
*/
struct dctx;

/** Opaque structure that holds a digested decompression dictionary.

    Created with @ref decompress_service::create_ddict and released
    with @ref decompress_service::free_ddict. A digested dictionary
    is read-only and may be shared by multiple contexts and threads.
*/
struct ddict;

/** Decompression parameter identifiers.

    These values identify parameters that can be set on a
    decompression context with @ref decompress_service::set_parameter.
    Parameters are sticky and remain valid for all following frames.
*/
enum class d_parameter
{
    /** Maximum window size accepted, as a power of 2.

        In streaming mode the decoder refuses to allocate a
        buffer larger than this, protecting the host from
        unreasonable memory requirements. Zero selects the
        default limit.
    */
    window_log_max = 100
};

/** Provides the Zstandard decompression API.

    This service interface exposes Zstandard decompression
    functionality through a set of virtual functions. Data
    can be decompressed in one shot with @ref decompress or
    @ref decompress_dctx when the content size is known, or
    incrementally with @ref decompress_stream.

    Most functions return a `std::size_t` which is either a
    byte count or an encoded error code. Test results with
    @ref is_error and convert them with @ref get_error_code
    or @ref get_error_name.

    @code
    // Example: Simple one-shot decompression
    auto& decompressor = boost::http::zstd::install_decompress_service(ctx);

    std::vector<char> compressed = get_compressed_data();
    auto size = decompressor.get_frame_content_size(
        compressed.data(), compressed.size());
    if (size == boost::http::zstd::content_size_error ||
        size == boost::http::zstd::content_size_unknown)
        return; // invalid frame, or streaming mode is required

    std::vector<char> output(size);
    std::size_t n = decompressor.decompress(
        output.data(), output.size(),
        compressed.data(), compressed.size());

    if (! decompressor.is_error(n))
    {
        // Use decompressed data
    }
    @endcode

    @code
    // Example: Streaming decompression
    auto* ctx = decompressor.create_dctx();

    std::vector<char> buf(decompressor.stream_out_size());
    boost::http::zstd::in_buffer in{ compressed.data(), compressed.size(), 0 };
    std::size_t rs;
    do
    {
        boost::http::zstd::out_buffer out{ buf.data(), buf.size(), 0 };
        rs = decompressor.decompress_stream(ctx, out, in);
        if (decompressor.is_error(rs))
            break;
        output.insert(output.end(), buf.data(), buf.data() + out.pos);
    }
    while (rs != 0);

    decompressor.free_dctx(ctx);
    @endcode
*/
struct BOOST_SYMBOL_VISIBLE
    decompress_service
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

    /** Decompress data in one call.

        The input must be the exact size of one or more
        complete frames; the output is their concatenation.

        @param dst Output buffer.
        @param dst_capacity Output buffer size; an upper bound
               of the decompressed size.
        @param src Compressed data.
        @param compressed_size Compressed data size.
        @return The decompressed size, or an error code.
    */
    virtual
    std::size_t
    decompress(
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t compressed_size) const noexcept = 0;

    /** Return the decompressed size recorded in a frame header.

        The size is an optional field which is always present
        for frames produced by the one-shot functions, and may
        be absent for frames produced in streaming mode. If the
        source is untrusted the value may be wrong; always check
        it against an application limit.

        @param src Start of a frame.
        @param src_size Number of bytes available; must cover
               the frame header.
        @return The content size, @ref content_size_unknown if
                it is not recorded, or @ref content_size_error
                if the header is invalid or incomplete.
    */
    virtual
    unsigned long long
    get_frame_content_size(
        void const* src,
        std::size_t src_size) const noexcept = 0;

    /** Return the compressed size of the first frame.

        This may need to scan the whole frame to find its end.

        @param src Start of a frame or skippable frame.
        @param src_size Number of bytes available; must cover
               the whole first frame.
        @return The compressed size of the first frame, or an
                error code.
    */
    virtual
    std::size_t
    find_frame_compressed_size(
        void const* src,
        std::size_t src_size) const noexcept = 0;

    /** Create a new decompression context.
        @return Pointer to the context, or nullptr on error.
    */
    virtual
    dctx*
    create_dctx() const noexcept = 0;

    /** Release a decompression context.
        @param ctx The context to release; may be nullptr.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    free_dctx(dctx* ctx) const noexcept = 0;

    /** Return the current memory usage of a decompression context.
        @param ctx The context.
        @return Memory usage in bytes.
    */
    virtual
    std::size_t
    sizeof_dctx(dctx const* ctx) const noexcept = 0;

    /** Return the valid bounds of a decompression parameter.
        @param param The parameter identifier.
        @return The bounds; test the `error` field with @ref is_error.
    */
    virtual
    bounds
    param_bounds(d_parameter param) const noexcept = 0;

    /** Set a decompression parameter.

        Parameters can only be set between frames, before
        decompression of the next frame starts.

        @param ctx The context.
        @param param The parameter identifier.
        @param value The parameter value.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    set_parameter(
        dctx* ctx,
        d_parameter param,
        int value) const noexcept = 0;

    /** Reset a decompression context.
        @param ctx The context.
        @param directive What to reset.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    reset(
        dctx* ctx,
        reset_directive directive) const noexcept = 0;

    /** Decompress data in one call using a context.

        Behaves like @ref decompress, honoring the parameters
        and dictionary set on the context.

        @param ctx The context.
        @param dst Output buffer.
        @param dst_capacity Output buffer size.
        @param src Compressed data.
        @param src_size Compressed data size.
        @return The decompressed size, or an error code.
    */
    virtual
    std::size_t
    decompress_dctx(
        dctx* ctx,
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t src_size) const noexcept = 0;

    /** Decompress data in streaming mode.

        Consumes input from `input` and writes output to
        `output`, advancing the `pos` field of each. If
        `input.pos < input.size` afterwards, the remaining
        input must be presented again. If the output buffer
        was filled completely, data may still be buffered
        internally; call again to flush it.

        @param ctx The context.
        @param output The output buffer.
        @param input The input buffer.
        @return Zero when a frame is completely decoded and
                fully flushed, an error code, or any other
                value which means more decoding or flushing
                is needed to complete the frame. The value
                is a hint for the next input size.
    */
    virtual
    std::size_t
    decompress_stream(
        dctx* ctx,
        out_buffer& output,
        in_buffer& input) const noexcept = 0;

    /** Return the recommended input buffer size for streaming.
        @return Size in bytes.
    */
    virtual
    std::size_t
    stream_in_size() const noexcept = 0;

    /** Return the recommended output buffer size for streaming.

        An output buffer of this size is guaranteed to be able
        to flush at least one complete block in all circumstances.

        @return Size in bytes.
    */
    virtual
    std::size_t
    stream_out_size() const noexcept = 0;

    /** Create a digested dictionary for decompression.
        @param dict The dictionary content; copied internally.
        @param dict_size The dictionary size.
        @return Pointer to the dictionary, or nullptr on error.
    */
    virtual
    ddict*
    create_ddict(
        void const* dict,
        std::size_t dict_size) const noexcept = 0;

    /** Release a digested dictionary.
        @param dict The dictionary to release; may be nullptr.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    free_ddict(ddict* dict) const noexcept = 0;

    /** Return the current memory usage of a digested dictionary.
        @param dict The dictionary.
        @return Memory usage in bytes.
    */
    virtual
    std::size_t
    sizeof_ddict(ddict const* dict) const noexcept = 0;

    /** Load a dictionary into a context.

        The content is copied and digested; it is used for all
        future frames until another dictionary is loaded or the
        parameters are reset. Loading a null or empty dictionary
        returns to no-dictionary mode.

        @param ctx The context.
        @param dict The dictionary content.
        @param dict_size The dictionary size.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    load_dictionary(
        dctx* ctx,
        void const* dict,
        std::size_t dict_size) const noexcept = 0;

    /** Reference a digested dictionary from a context.

        The dictionary is only referenced and must outlive its
        use by the context. Referencing nullptr returns to
        no-dictionary mode.

        @param ctx The context.
        @param dict The digested dictionary.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    ref_ddict(
        dctx* ctx,
        ddict const* dict) const noexcept = 0;

    /** Reference a prefix for the next frame.

        The prefix must be the same raw content used with
        @ref compress_service::ref_prefix during compression.
        It is used once and discarded when the frame ends.
        The buffer is only referenced and must remain valid
        and unmodified until then.

        @param ctx The context.
        @param prefix The prefix content.
        @param prefix_size The prefix size.
        @return Zero, or an error code.
    */
    virtual
    std::size_t
    ref_prefix(
        dctx* ctx,
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

    /** Return the dictionary ID required to decompress a frame.
        @param src Start of a frame.
        @param src_size Number of bytes available.
        @return The dictionary ID, or zero if the frame needs
                no dictionary, the ID was omitted, the header
                is incomplete, or this is not a frame.
    */
    virtual
    unsigned
    get_dict_id_from_frame(
        void const* src,
        std::size_t src_size) const noexcept = 0;

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
