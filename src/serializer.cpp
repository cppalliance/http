//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Christian Mazakas
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/detail/except.hpp>
#include <boost/http_proto/detail/header.hpp>
#include <boost/http_proto/message_base.hpp>
#include <boost/http_proto/serializer.hpp>

#include "src/detail/array_of_const_buffers.hpp"
#include "src/detail/brotli_filter_base.hpp"
#include "src/detail/buffer_utils.hpp"
#include "src/detail/zlib_filter_base.hpp"

#include <boost/buffers/circular_buffer.hpp>
#include <boost/buffers/copy.hpp>
#include <boost/core/bit.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/capy/brotli/encode.hpp>
#include <boost/capy/polystore.hpp>
#include <boost/capy/zlib/compression_method.hpp>
#include <boost/capy/zlib/compression_strategy.hpp>
#include <boost/capy/zlib/deflate.hpp>
#include <boost/capy/zlib/error.hpp>
#include <boost/capy/zlib/flush.hpp>

#include <stddef.h>

namespace boost {
namespace http_proto {

namespace {

const
buffers::const_buffer
crlf_and_final_chunk = {"\r\n0\r\n\r\n", 7};

const
buffers::const_buffer
crlf = {"\r\n", 2};

const
buffers::const_buffer
final_chunk = {"0\r\n\r\n", 5};

constexpr
std::uint8_t
chunk_header_len(
    std::size_t max_chunk_size) noexcept
{
    return
        static_cast<uint8_t>(
            (core::bit_width(max_chunk_size) + 3) / 4 +
            2); // crlf
};

void
write_chunk_header(
    const buffers::mutable_buffer_pair& mbs,
    std::size_t size) noexcept
{
    static constexpr char hexdig[] =
        "0123456789ABCDEF";
    char buf[18];
    auto p = buf + 16;
    auto const n = buffers::size(mbs);
    for(std::size_t i = n - 2; i--;)
    {
        *--p = hexdig[size & 0xf];
        size >>= 4;
    }
    buf[16] = '\r';
    buf[17] = '\n';
    auto copied = buffers::copy(
        mbs,
        buffers::const_buffer(p, n));
    ignore_unused(copied);
    BOOST_ASSERT(copied == n);
}

//------------------------------------------------

class zlib_filter
    : public detail::zlib_filter_base
{
    capy::zlib::deflate_service& svc_;

public:
    zlib_filter(
        const capy::polystore& ctx,
        http_proto::detail::workspace& ws,
        int comp_level,
        int window_bits,
        int mem_level)
        : zlib_filter_base(ws)
        , svc_(ctx.get<capy::zlib::deflate_service>())
    {
        system::error_code ec = static_cast<capy::zlib::error>(svc_.init2(
            strm_,
            comp_level,
            capy::zlib::deflated,
            window_bits,
            mem_level,
            capy::zlib::default_strategy));
        if(ec != capy::zlib::error::ok)
            detail::throw_system_error(ec);
    }

private:
    virtual
    std::size_t
    min_out_buffer() const noexcept override
    {
        // Prevents deflate from producing
        // zero output due to small buffer
        return 8;
    }

    virtual
    results
    do_process(
        buffers::mutable_buffer out,
        buffers::const_buffer in,
        bool more) noexcept override
    {
        strm_.next_out  = static_cast<unsigned char*>(out.data());
        strm_.avail_out = saturate_cast(out.size());
        strm_.next_in   = static_cast<unsigned char*>(const_cast<void *>(in.data()));
        strm_.avail_in  = saturate_cast(in.size());

        auto rs = static_cast<capy::zlib::error>(
            svc_.deflate(
                strm_,
                more ? capy::zlib::no_flush : capy::zlib::finish));

        results rv;
        rv.out_bytes = saturate_cast(out.size()) - strm_.avail_out;
        rv.in_bytes  = saturate_cast(in.size()) - strm_.avail_in;
        rv.finished  = (rs == capy::zlib::error::stream_end);

        if(rs < capy::zlib::error::ok && rs != capy::zlib::error::buf_err)
            rv.ec = rs;

        return rv;
    }
};

class brotli_filter
    : public detail::brotli_filter_base
{
    capy::brotli::encode_service& svc_;
    capy::brotli::encoder_state* state_;

public:
    brotli_filter(
        const capy::polystore& ctx,
        http_proto::detail::workspace&,
        std::uint32_t comp_quality,
        std::uint32_t comp_window)
        : svc_(ctx.get<capy::brotli::encode_service>())
    {
        // TODO: use custom allocator
        state_ = svc_.create_instance(nullptr, nullptr, nullptr);
        if(!state_)
            detail::throw_bad_alloc();
        using encoder_parameter = capy::brotli::encoder_parameter;
        svc_.set_parameter(state_, encoder_parameter::quality, comp_quality);
        svc_.set_parameter(state_, encoder_parameter::lgwin, comp_window);
    }

    ~brotli_filter()
    {
        svc_.destroy_instance(state_);
    }

private:
    virtual
    results
    do_process(
        buffers::mutable_buffer out,
        buffers::const_buffer in,
        bool more) noexcept override
    {
        auto* next_in = reinterpret_cast<const std::uint8_t*>(in.data());
        auto available_in = in.size();
        auto* next_out = reinterpret_cast<std::uint8_t*>(out.data());
        auto available_out = out.size();

        using encoder_operation = 
            capy::brotli::encoder_operation;

        bool rs = svc_.compress_stream(
            state_,
            more ? encoder_operation::process : encoder_operation::finish,
            &available_in,
            &next_in,
            &available_out,
            &next_out,
            nullptr);

        results rv;
        rv.in_bytes  = in.size()  - available_in;
        rv.out_bytes = out.size() - available_out;
        rv.finished  = svc_.is_finished(state_);

        // TODO: use proper error code
        if(rs == false)
            rv.ec = error::bad_payload;

        return rv;
    }
};

template<class UInt>
std::size_t
clamp(
    UInt x,
    std::size_t limit = (std::numeric_limits<
        std::size_t>::max)()) noexcept
{
    if(x >= limit)
        return limit;
    return static_cast<std::size_t>(x);
}

class serializer_service
{
public:
    serializer::config cfg;
    std::size_t space_needed = 0;

    serializer_service(
        serializer::config const& cfg_)
        : cfg(cfg_)
    {
        space_needed += cfg.payload_buffer;
        space_needed += cfg.max_type_erase;

        if(cfg.apply_deflate_encoder || cfg.apply_gzip_encoder)
        {
            // TODO: Account for the number of allocations and
            // their overhead in the workspace.

            // https://www.zlib.net/zlib_tech.html
            space_needed +=
                (1 << (cfg.zlib_window_bits + 2)) +
                (1 << (cfg.zlib_mem_level + 9)) +
                (6 * 1024) +
                #ifdef __s390x__
                5768 +
                #endif
                detail::workspace::space_needed<zlib_filter>();
        }
    }
};

} // namespace

//------------------------------------------------

void
install_serializer_service(
    capy::polystore& ctx,
    serializer::config const& cfg)
{
    ctx.emplace<serializer_service>(cfg);
}

//------------------------------------------------

class serializer::impl
{
    friend stream;

    enum class state
    {
        reset,
        initialized,
        headers_set,
        header,
        body
    };

    enum class style
    {
        empty,
        buffers,
        source,
        stream
    };

    const capy::polystore& ctx_;
    serializer_service& svc_;
    detail::workspace ws_;

    detail::filter* filter_ = nullptr;
    cbs_gen* cbs_gen_ = nullptr;
    source* source_ = nullptr;

    buffers::circular_buffer out_;
    buffers::circular_buffer in_;
    detail::array_of_const_buffers prepped_;
    buffers::const_buffer tmp_;

    state state_ = state::initialized;
    style style_ = style::empty;
    uint8_t chunk_header_len_ = 0;
    bool more_input_ = false;
    bool is_chunked_ = false;
    bool needs_exp100_continue_ = false;
    bool filter_done_ = false;

    // Stored header buffer from set_headers
    char const* header_cbuf_ = nullptr;
    std::size_t header_size_ = 0;

public:
    impl(const capy::polystore& ctx)
        : ctx_(ctx)
        , svc_(ctx_.get<serializer_service>())
        , ws_(svc_.space_needed)
    {
    }

    void
    reset() noexcept
    {
        ws_.clear();
        state_ = state::initialized;
    }

    auto
    prepare() ->
        system::result<const_buffers_type>
    {
        // Precondition violation
        if(state_ < state::header)
            detail::throw_logic_error();

        // Expect: 100-continue
        if(needs_exp100_continue_)
        {
            if(!is_header_done())
                return const_buffers_type(
                    prepped_.begin(),
                    1); // limit to header

            needs_exp100_continue_ = false;

            BOOST_HTTP_PROTO_RETURN_EC(
                error::expect_100_continue);
        }

        if(!filter_)
        {
            switch(style_)
            {
            case style::empty:
                break;

            case style::buffers:
            {
                // add more buffers if prepped_ is half empty.
                if(more_input_ &&
                    prepped_.capacity() >= prepped_.size())
                {
                    prepped_.slide_to_front();
                    while(prepped_.capacity() != 0)
                    {
                        auto buf = cbs_gen_->next();
                        if(buf.size() == 0)
                            break;
                        prepped_.append(buf);
                    }
                    if(cbs_gen_->is_empty())
                    {
                        if(is_chunked_)
                        {
                            if(prepped_.capacity() != 0)
                            {
                                prepped_.append(
                                    crlf_and_final_chunk);
                                more_input_ = false;
                            }
                        }
                        else
                        {
                            more_input_ = false;
                        }
                    }
                }
                return detail::make_span(prepped_);
            }

            case style::source:
            {
                if(out_capacity() == 0 || !more_input_)
                    break;

                const auto rs = source_->read(
                    out_prepare());

                out_commit(rs.bytes);

                if(rs.ec.failed())
                {
                    ws_.clear();
                    state_ = state::reset;
                    return rs.ec;
                }

                if(rs.finished)
                {
                    more_input_ = false;
                    out_finish();
                }

                break;
            }

            case style::stream:
                if(out_.size() == 0 && is_header_done() && more_input_)
                    BOOST_HTTP_PROTO_RETURN_EC(
                        error::need_data);
                break;
            }
        }
        else // filter
        {
            switch(style_)
            {
            case style::empty:
            {
                if(out_capacity() == 0 || filter_done_)
                    break;

                const auto rs = filter_->process(
                    detail::make_span(out_prepare()),
                    {}, // empty input
                    false);

                if(rs.ec.failed())
                {
                    ws_.clear();
                    state_ = state::reset;
                    return rs.ec;
                }

                out_commit(rs.out_bytes);

                if(rs.finished)
                {
                    filter_done_ = true;
                    out_finish();
                }

                break;
            }

            case style::buffers:
            {
                while(out_capacity() != 0 && !filter_done_)
                {
                    if(more_input_ && tmp_.size() == 0)
                    {
                        tmp_ = cbs_gen_->next();
                        if(tmp_.size() == 0) // cbs_gen_ is empty
                            more_input_ = false;
                    }

                    const auto rs = filter_->process(
                        detail::make_span(out_prepare()),
                        {{ {tmp_}, {} }},
                        more_input_);

                    if(rs.ec.failed())
                    {
                        ws_.clear();
                        state_ = state::reset;
                        return rs.ec;
                    }

                    buffers::remove_prefix(tmp_, rs.in_bytes);
                    out_commit(rs.out_bytes);

                    if(rs.out_short)
                        break;

                    if(rs.finished)
                    {
                        filter_done_ = true;
                        out_finish();
                    }
                }
                break;
            }

            case style::source:
            {
                while(out_capacity() != 0 && !filter_done_)
                {
                    if(more_input_ && in_.capacity() != 0)
                    {
                        const auto rs = source_->read(
                            in_.prepare(in_.capacity()));
                        if(rs.ec.failed())
                        {
                            ws_.clear();
                            state_ = state::reset;
                            return rs.ec;
                        }
                        if(rs.finished)
                            more_input_ = false;
                        in_.commit(rs.bytes);
                    }

                    const auto rs = filter_->process(
                        detail::make_span(out_prepare()),
                        in_.data(),
                        more_input_);

                    if(rs.ec.failed())
                    {
                        ws_.clear();
                        state_ = state::reset;
                        return rs.ec;
                    }

                    in_.consume(rs.in_bytes);
                    out_commit(rs.out_bytes);

                    if(rs.out_short)
                        break;

                    if(rs.finished)
                    {
                        filter_done_ = true;
                        out_finish();
                    }
                }
                break;
            }

            case style::stream:
            {
                if(out_capacity() == 0 || filter_done_)
                    break;

                const auto rs = filter_->process(
                    detail::make_span(out_prepare()),
                    in_.data(),
                    more_input_);

                if(rs.ec.failed())
                {
                    ws_.clear();
                    state_ = state::reset;
                    return rs.ec;
                }

                in_.consume(rs.in_bytes);
                out_commit(rs.out_bytes);

                if(rs.finished)
                {
                    filter_done_ = true;
                    out_finish();
                }

                if(out_.size() == 0 && is_header_done() && more_input_)
                    BOOST_HTTP_PROTO_RETURN_EC(
                        error::need_data);
                break;
            }
            }
        }

        prepped_.reset(!is_header_done());
        for(auto const& cb : out_.data())
        {
            if(cb.size() != 0)
                prepped_.append(cb);
        }
        return detail::make_span(prepped_);
    }

    void
    consume(
        std::size_t n)
    {
        // Precondition violation
        if(state_ < state::header)
            detail::throw_logic_error();

        if(!is_header_done())
        {
            const auto header_remain =
                prepped_[0].size();
            if(n < header_remain)
            {
                prepped_.consume(n);
                return;
            }
            n -= header_remain;
            prepped_.consume(header_remain);
            state_ = state::body;
        }

        prepped_.consume(n);

        // no-op when out_ is not in use
        out_.consume(n);

        if(!prepped_.empty())
            return;

        if(more_input_)
            return;

        if(filter_ && !filter_done_)
            return;

        if(needs_exp100_continue_)
            return;

        // ready for next message
        reset();
    }

    void
    set_headers(
        message_base const& m)
    {
        // Precondition violation
        if(state_ != state::initialized)
            detail::throw_logic_error();

        // TODO: To uphold the strong exception guarantee,
        // `state_` must be reset to `state::initialized` if an
        // exception is thrown during the start operation.
        state_ = state::headers_set;

        // VFALCO what do we do with
        // metadata error code failures?
        // m.h_.md.maybe_throw();

        auto const& md = m.metadata();
        needs_exp100_continue_ = md.expect.is_100_continue;

        // Transfer-Encoding
        is_chunked_ = md.transfer_encoding.is_chunked;

        // Content-Encoding
        switch (md.content_encoding.coding)
        {
        case content_coding::deflate:
            if(!svc_.cfg.apply_deflate_encoder)
                goto no_filter;
            filter_ = &ws_.emplace<zlib_filter>(
                ctx_,
                ws_,
                svc_.cfg.zlib_comp_level,
                svc_.cfg.zlib_window_bits,
                svc_.cfg.zlib_mem_level);
            filter_done_ = false;
            break;

        case content_coding::gzip:
            if(!svc_.cfg.apply_gzip_encoder)
                goto no_filter;
            filter_ = &ws_.emplace<zlib_filter>(
                ctx_,
                ws_,
                svc_.cfg.zlib_comp_level,
                svc_.cfg.zlib_window_bits + 16,
                svc_.cfg.zlib_mem_level);
            filter_done_ = false;
            break;

        case content_coding::br:
            if(!svc_.cfg.apply_brotli_encoder)
                goto no_filter;
            filter_ = &ws_.emplace<brotli_filter>(
                ctx_,
                ws_,
                svc_.cfg.brotli_comp_quality,
                svc_.cfg.brotli_comp_window);
            filter_done_ = false;
            break;

        no_filter:
        default:
            filter_ = nullptr;
            break;
        }

        // Store header buffer for later use
        header_cbuf_ = m.h_.cbuf;
        header_size_ = m.h_.size;
    }

    void
    start_empty(
        message_base const& m)
    {
        set_headers(m);
        start_empty_impl();
    }

    void
    start_buffers(
        message_base const& m,
        cbs_gen& cbs_gen)
    {
        set_headers(m);
        start_buffers_impl(cbs_gen);
    }

    void
    start_source(
        message_base const& m,
        source& source)
    {
        set_headers(m);
        start_source_impl(source);
    }

    stream
    start_stream(message_base const& m)
    {
        set_headers(m);
        return start_stream_impl();
    }

    // New methods that use stored header from set_headers()

    void
    start_empty_impl()
    {
        // Precondition violation
        if(state_ != state::headers_set)
            detail::throw_logic_error();

        state_ = state::header;
        style_ = style::empty;

        prepped_ = make_array(
            1 + // header
            2); // out buffer pairs

        out_init();

        if(!filter_)
            out_finish();

        prepped_.append({ header_cbuf_, header_size_ });
        more_input_ = false;
    }

    void
    start_buffers_impl(
        cbs_gen& cbs_gen)
    {
        // Precondition violation
        if(state_ != state::headers_set)
            detail::throw_logic_error();

        state_ = state::header;
        style_ = style::buffers;
        cbs_gen_ = &cbs_gen;

        if(!filter_)
        {
            auto stats = cbs_gen_->stats();
            auto batch_size = clamp(stats.count, 16);

            prepped_ = make_array(
                1 + // header
                batch_size + // buffers
                (is_chunked_ ? 2 : 0)); // chunk header + final chunk

            prepped_.append({ header_cbuf_, header_size_ });
            more_input_ = (batch_size != 0);

            if(is_chunked_)
            {
                if(!more_input_)
                {
                    prepped_.append(final_chunk);
                }
                else
                {
                    auto h_len = chunk_header_len(stats.size);
                    buffers::mutable_buffer mb(
                        ws_.reserve_front(h_len), h_len);
                    write_chunk_header({{ {mb}, {} }}, stats.size);
                    prepped_.append(mb);
                }
            }
            return;
        }

        // filter

        prepped_ = make_array(
            1 + // header
            2); // out buffer pairs

        out_init();

        prepped_.append({ header_cbuf_, header_size_ });
        tmp_ = {};
        more_input_ = true;
    }

    void
    start_source_impl(
        source& source)
    {
        // Precondition violation
        if(state_ != state::headers_set)
            detail::throw_logic_error();

        state_ = state::header;
        style_ = style::source;
        source_ = &source;

        prepped_ = make_array(
            1 + // header
            2); // out buffer pairs

        if(filter_)
        {
            // TODO: smarter buffer distribution
            auto const n = (ws_.size() - 1) / 2;
            in_ = { ws_.reserve_front(n), n };
        }

        out_init();

        prepped_.append({ header_cbuf_, header_size_ });
        more_input_ = true;
    }

    stream
    start_stream_impl()
    {
        // Precondition violation
        if(state_ != state::headers_set)
            detail::throw_logic_error();

        state_ = state::header;
        style_ = style::stream;

        prepped_ = make_array(
            1 + // header
            2); // out buffer pairs

        if(filter_)
        {
            // TODO: smarter buffer distribution
            auto const n = (ws_.size() - 1) / 2;
            in_ = { ws_.reserve_front(n), n };
        }

        out_init();

        prepped_.append({ header_cbuf_, header_size_ });
        more_input_ = true;
        return stream{ this };
    }

    bool
    is_done() const noexcept
    {
        return state_ == state::initialized;
    }

    bool
    is_headers_set() const noexcept
    {
        return state_ == state::headers_set;
    }

    detail::workspace&
    ws() noexcept
    {
        return ws_;
    }

private:
    bool
    is_header_done() const noexcept
    {
        return state_ == state::body;
    }

    detail::array_of_const_buffers
    make_array(std::size_t n)
    {
        BOOST_ASSERT(n <= std::uint16_t(-1));

        return {
            ws_.push_array(n,
                buffers::const_buffer{}),
            static_cast<std::uint16_t>(n) };
    }

    void
    out_init()
    {
        // use all the remaining buffer
        auto const n = ws_.size() - 1;
        out_ = { ws_.reserve_front(n), n };
        chunk_header_len_ =
            chunk_header_len(out_.capacity());
        if(out_capacity() == 0)
            detail::throw_length_error();
    }

    buffers::mutable_buffer_pair
    out_prepare() noexcept
    {
        auto mbp = out_.prepare(out_.capacity());
        if(is_chunked_)
        {
            buffers::remove_prefix(
                mbp, chunk_header_len_);
            buffers::remove_suffix(
                mbp, crlf_and_final_chunk.size());
        }
        return mbp;
    }

    void
    out_commit(
        std::size_t n) noexcept
    {
        if(is_chunked_)
        {
            if(n == 0)
                return;

            write_chunk_header(out_.prepare(chunk_header_len_), n);
            out_.commit(chunk_header_len_);

            out_.prepare(n);
            out_.commit(n);

            buffers::copy(out_.prepare(crlf.size()), crlf);
            out_.commit(crlf.size());
        }
        else
        {
            out_.commit(n);
        }
    }

    std::size_t
    out_capacity() const noexcept
    {
        if(is_chunked_)
        {
            auto const overhead = chunk_header_len_ +
                crlf_and_final_chunk.size();
            if(out_.capacity() < overhead)
                return 0;
            return out_.capacity() - overhead;
        }
        return out_.capacity();
    }

    void
    out_finish() noexcept
    {
        if(is_chunked_)
        {
            buffers::copy(
                out_.prepare(final_chunk.size()), final_chunk);
            out_.commit(final_chunk.size());
        }
    }
};

//------------------------------------------------

serializer::
~serializer()
{
    delete impl_;
}

serializer::
serializer(serializer&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = nullptr;
}

serializer&
serializer::
operator=(serializer&& other) noexcept
{
    if(this != &other)
    {
        delete impl_;
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

serializer::
serializer(capy::polystore& ctx)
    : impl_(new impl(ctx))
{
    // TODO: use a single allocation for
    // impl and workspace buffer.
}

void
serializer::
reset() noexcept
{
    BOOST_ASSERT(impl_);
    impl_->reset();
}

void
serializer::
start(message_base const& m)
{
    BOOST_ASSERT(impl_);
    impl_->start_empty(m);
}

void
serializer::
set_headers(message_base const& m)
{
    BOOST_ASSERT(impl_);
    impl_->set_headers(m);
}

void
serializer::
start()
{
    BOOST_ASSERT(impl_);
    impl_->start_empty_impl();
}

auto
serializer::
start_stream(
    message_base const& m) -> stream
{
    BOOST_ASSERT(impl_);
    return impl_->start_stream(m);
}

auto
serializer::
start_stream() -> stream
{
    BOOST_ASSERT(impl_);
    return impl_->start_stream_impl();
}

auto
serializer::
prepare() ->
    system::result<const_buffers_type>
{
    BOOST_ASSERT(impl_);
    return impl_->prepare();
}

void
serializer::
consume(std::size_t n)
{
    BOOST_ASSERT(impl_);
    impl_->consume(n);
}

bool
serializer::
is_done() const noexcept
{
    BOOST_ASSERT(impl_);
    return impl_->is_done();
}

bool
serializer::
is_headers_set() const noexcept
{
    BOOST_ASSERT(impl_);
    return impl_->is_headers_set();
}

//------------------------------------------------

detail::workspace&
serializer::
ws()
{
    BOOST_ASSERT(impl_);
    return impl_->ws();
}

void
serializer::
start_buffers_impl(
    cbs_gen& cbs_gen)
{
    BOOST_ASSERT(impl_);
    impl_->start_buffers_impl(cbs_gen);
}

void
serializer::
start_source_impl(
    source& source)
{
    BOOST_ASSERT(impl_);
    impl_->start_source_impl(source);
}

//------------------------------------------------

std::size_t
serializer::
stream::
capacity() const
{
    // Precondition violation
    if(!is_open())
        detail::throw_logic_error();

    if(impl_->filter_)
        return impl_->in_.capacity();

    return impl_->out_capacity();
}

auto
serializer::
stream::
prepare() ->
    mutable_buffers_type
{
    // Precondition violation
    if(!is_open())
        detail::throw_logic_error();

    if(impl_->filter_)
        return impl_->in_.prepare(
            impl_->in_.capacity());

    return impl_->out_prepare();
}

void
serializer::
stream::
commit(std::size_t n)
{
    // Precondition violation
    if(!is_open())
        detail::throw_logic_error();

    // Precondition violation
    if(n > capacity())
        detail::throw_invalid_argument();

    if(impl_->filter_)
        return impl_->in_.commit(n);

    impl_->out_commit(n);
}

void
serializer::
stream::
close() noexcept
{
    if(!is_open())
        return; // no-op;

    if(!impl_->filter_)
        impl_->out_finish();

    impl_->more_input_ = false;
    impl_ = nullptr;
}

} // http_proto
} // boost
