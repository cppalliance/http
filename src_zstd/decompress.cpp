//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/zstd/decompress.hpp>
#include <boost/capy/ex/system_context.hpp>

#include <zstd.h>
#include <zstd_errors.h>

#if ZSTD_VERSION_NUMBER < 10400
# error "Boost.HTTP requires zstd 1.4.0 or later"
#endif

namespace boost {
namespace http {
namespace zstd {

class decompress_service_impl
    : public decompress_service
{
public:
    using key_type = decompress_service;

    explicit
    decompress_service_impl(
        capy::execution_context&) noexcept
    {
    }

    ~decompress_service_impl()
    {
    }

    unsigned
    version_number() const noexcept override
    {
        return ZSTD_versionNumber();
    }

    char const*
    version_string() const noexcept override
    {
        return ZSTD_versionString();
    }

    std::size_t
    decompress(
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t compressed_size) const noexcept override
    {
        return ZSTD_decompress(
            dst,
            dst_capacity,
            src,
            compressed_size);
    }

    unsigned long long
    get_frame_content_size(
        void const* src,
        std::size_t src_size) const noexcept override
    {
        return ZSTD_getFrameContentSize(src, src_size);
    }

    std::size_t
    find_frame_compressed_size(
        void const* src,
        std::size_t src_size) const noexcept override
    {
        return ZSTD_findFrameCompressedSize(src, src_size);
    }

    dctx*
    create_dctx() const noexcept override
    {
        return reinterpret_cast<dctx*>(
            ZSTD_createDCtx());
    }

    std::size_t
    free_dctx(dctx* ctx) const noexcept override
    {
        return ZSTD_freeDCtx(
            reinterpret_cast<ZSTD_DCtx*>(ctx));
    }

    std::size_t
    sizeof_dctx(dctx const* ctx) const noexcept override
    {
        return ZSTD_sizeof_DCtx(
            reinterpret_cast<ZSTD_DCtx const*>(ctx));
    }

    bounds
    param_bounds(d_parameter param) const noexcept override
    {
        auto const b = ZSTD_dParam_getBounds(
            static_cast<ZSTD_dParameter>(param));
        return bounds{ b.error, b.lowerBound, b.upperBound };
    }

    std::size_t
    set_parameter(
        dctx* ctx,
        d_parameter param,
        int value) const noexcept override
    {
        return ZSTD_DCtx_setParameter(
            reinterpret_cast<ZSTD_DCtx*>(ctx),
            static_cast<ZSTD_dParameter>(param),
            value);
    }

    std::size_t
    reset(
        dctx* ctx,
        reset_directive directive) const noexcept override
    {
        return ZSTD_DCtx_reset(
            reinterpret_cast<ZSTD_DCtx*>(ctx),
            static_cast<ZSTD_ResetDirective>(directive));
    }

    std::size_t
    decompress_dctx(
        dctx* ctx,
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t src_size) const noexcept override
    {
        return ZSTD_decompressDCtx(
            reinterpret_cast<ZSTD_DCtx*>(ctx),
            dst,
            dst_capacity,
            src,
            src_size);
    }

    std::size_t
    decompress_stream(
        dctx* ctx,
        out_buffer& output,
        in_buffer& input) const noexcept override
    {
        ZSTD_outBuffer out{ output.dst, output.size, output.pos };
        ZSTD_inBuffer in{ input.src, input.size, input.pos };
        auto const rs = ZSTD_decompressStream(
            reinterpret_cast<ZSTD_DCtx*>(ctx),
            &out,
            &in);
        output.pos = out.pos;
        input.pos = in.pos;
        return rs;
    }

    std::size_t
    stream_in_size() const noexcept override
    {
        return ZSTD_DStreamInSize();
    }

    std::size_t
    stream_out_size() const noexcept override
    {
        return ZSTD_DStreamOutSize();
    }

    ddict*
    create_ddict(
        void const* dict,
        std::size_t dict_size) const noexcept override
    {
        return reinterpret_cast<ddict*>(
            ZSTD_createDDict(dict, dict_size));
    }

    std::size_t
    free_ddict(ddict* dict) const noexcept override
    {
        return ZSTD_freeDDict(
            reinterpret_cast<ZSTD_DDict*>(dict));
    }

    std::size_t
    sizeof_ddict(ddict const* dict) const noexcept override
    {
        return ZSTD_sizeof_DDict(
            reinterpret_cast<ZSTD_DDict const*>(dict));
    }

    std::size_t
    load_dictionary(
        dctx* ctx,
        void const* dict,
        std::size_t dict_size) const noexcept override
    {
        return ZSTD_DCtx_loadDictionary(
            reinterpret_cast<ZSTD_DCtx*>(ctx),
            dict,
            dict_size);
    }

    std::size_t
    ref_ddict(
        dctx* ctx,
        ddict const* dict) const noexcept override
    {
        return ZSTD_DCtx_refDDict(
            reinterpret_cast<ZSTD_DCtx*>(ctx),
            reinterpret_cast<ZSTD_DDict const*>(dict));
    }

    std::size_t
    ref_prefix(
        dctx* ctx,
        void const* prefix,
        std::size_t prefix_size) const noexcept override
    {
        return ZSTD_DCtx_refPrefix(
            reinterpret_cast<ZSTD_DCtx*>(ctx),
            prefix,
            prefix_size);
    }

    unsigned
    get_dict_id_from_dict(
        void const* dict,
        std::size_t dict_size) const noexcept override
    {
        return ZSTD_getDictID_fromDict(dict, dict_size);
    }

    unsigned
    get_dict_id_from_frame(
        void const* src,
        std::size_t src_size) const noexcept override
    {
        return ZSTD_getDictID_fromFrame(src, src_size);
    }

    bool
    is_error(std::size_t result) const noexcept override
    {
        return ZSTD_isError(result) != 0;
    }

    error
    get_error_code(std::size_t result) const noexcept override
    {
        return static_cast<error>(
            ZSTD_getErrorCode(result));
    }

    char const*
    get_error_name(std::size_t result) const noexcept override
    {
        return ZSTD_getErrorName(result);
    }

    char const*
    error_string(error c) const noexcept override
    {
        return ZSTD_getErrorString(
            static_cast<ZSTD_ErrorCode>(c));
    }
};

decompress_service&
install_decompress_service(capy::execution_context& ctx)
{
    return ctx.make_service<decompress_service_impl>();
}

} // zstd
} // http
} // boost
