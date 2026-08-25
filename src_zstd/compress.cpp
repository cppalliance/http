//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/zstd/compress.hpp>
#include <boost/capy/ex/system_context.hpp>

#include <zstd.h>
#include <zstd_errors.h>

#if ZSTD_VERSION_NUMBER < 10400
# error "Boost.HTTP requires zstd 1.4.0 or later"
#endif

namespace boost {
namespace http {
namespace zstd {

class compress_service_impl
    : public compress_service
{
public:
    using key_type = compress_service;

    explicit
    compress_service_impl(
        capy::execution_context&) noexcept
    {
    }

    ~compress_service_impl()
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

    int
    min_level() const noexcept override
    {
        return ZSTD_minCLevel();
    }

    int
    max_level() const noexcept override
    {
        return ZSTD_maxCLevel();
    }

    int
    default_level() const noexcept override
    {
#if ZSTD_VERSION_NUMBER >= 10500
        return ZSTD_defaultCLevel();
#else
        return ZSTD_CLEVEL_DEFAULT;
#endif
    }

    std::size_t
    compress_bound(std::size_t src_size) const noexcept override
    {
        return ZSTD_compressBound(src_size);
    }

    std::size_t
    compress(
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t src_size,
        int level) const noexcept override
    {
        return ZSTD_compress(
            dst,
            dst_capacity,
            src,
            src_size,
            level);
    }

    cctx*
    create_cctx() const noexcept override
    {
        return reinterpret_cast<cctx*>(
            ZSTD_createCCtx());
    }

    std::size_t
    free_cctx(cctx* ctx) const noexcept override
    {
        return ZSTD_freeCCtx(
            reinterpret_cast<ZSTD_CCtx*>(ctx));
    }

    std::size_t
    sizeof_cctx(cctx const* ctx) const noexcept override
    {
        return ZSTD_sizeof_CCtx(
            reinterpret_cast<ZSTD_CCtx const*>(ctx));
    }

    bounds
    param_bounds(c_parameter param) const noexcept override
    {
        auto const b = ZSTD_cParam_getBounds(
            static_cast<ZSTD_cParameter>(param));
        return bounds{ b.error, b.lowerBound, b.upperBound };
    }

    std::size_t
    set_parameter(
        cctx* ctx,
        c_parameter param,
        int value) const noexcept override
    {
        return ZSTD_CCtx_setParameter(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
            static_cast<ZSTD_cParameter>(param),
            value);
    }

    std::size_t
    set_pledged_src_size(
        cctx* ctx,
        unsigned long long pledged_src_size) const noexcept override
    {
        return ZSTD_CCtx_setPledgedSrcSize(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
            pledged_src_size);
    }

    std::size_t
    reset(
        cctx* ctx,
        reset_directive directive) const noexcept override
    {
        return ZSTD_CCtx_reset(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
            static_cast<ZSTD_ResetDirective>(directive));
    }

    std::size_t
    compress2(
        cctx* ctx,
        void* dst,
        std::size_t dst_capacity,
        void const* src,
        std::size_t src_size) const noexcept override
    {
        return ZSTD_compress2(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
            dst,
            dst_capacity,
            src,
            src_size);
    }

    std::size_t
    compress_stream(
        cctx* ctx,
        out_buffer& output,
        in_buffer& input,
        end_directive end_op) const noexcept override
    {
        ZSTD_outBuffer out{ output.dst, output.size, output.pos };
        ZSTD_inBuffer in{ input.src, input.size, input.pos };
        auto const rs = ZSTD_compressStream2(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
            &out,
            &in,
            static_cast<ZSTD_EndDirective>(end_op));
        output.pos = out.pos;
        input.pos = in.pos;
        return rs;
    }

    std::size_t
    stream_in_size() const noexcept override
    {
        return ZSTD_CStreamInSize();
    }

    std::size_t
    stream_out_size() const noexcept override
    {
        return ZSTD_CStreamOutSize();
    }

    cdict*
    create_cdict(
        void const* dict,
        std::size_t dict_size,
        int level) const noexcept override
    {
        return reinterpret_cast<cdict*>(
            ZSTD_createCDict(dict, dict_size, level));
    }

    std::size_t
    free_cdict(cdict* dict) const noexcept override
    {
        return ZSTD_freeCDict(
            reinterpret_cast<ZSTD_CDict*>(dict));
    }

    std::size_t
    sizeof_cdict(cdict const* dict) const noexcept override
    {
        return ZSTD_sizeof_CDict(
            reinterpret_cast<ZSTD_CDict const*>(dict));
    }

    std::size_t
    load_dictionary(
        cctx* ctx,
        void const* dict,
        std::size_t dict_size) const noexcept override
    {
        return ZSTD_CCtx_loadDictionary(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
            dict,
            dict_size);
    }

    std::size_t
    ref_cdict(
        cctx* ctx,
        cdict const* dict) const noexcept override
    {
        return ZSTD_CCtx_refCDict(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
            reinterpret_cast<ZSTD_CDict const*>(dict));
    }

    std::size_t
    ref_prefix(
        cctx* ctx,
        void const* prefix,
        std::size_t prefix_size) const noexcept override
    {
        return ZSTD_CCtx_refPrefix(
            reinterpret_cast<ZSTD_CCtx*>(ctx),
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

compress_service&
install_compress_service(capy::execution_context& ctx)
{
    return ctx.make_service<compress_service_impl>();
}

void
install_zstd_service()
{
    install_compress_service(capy::get_system_context());
    install_decompress_service(capy::get_system_context());
}

} // zstd
} // http
} // boost
