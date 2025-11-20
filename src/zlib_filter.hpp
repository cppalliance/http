//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/capy/zlib/error.hpp>
#include <boost/capy/zlib/inflate.hpp>
#include "src/detail/zlib_filter_base.hpp"

namespace boost {
namespace http_proto {

class zlib_filter
    : public detail::zlib_filter_base
{
    capy::zlib::inflate_service& svc_;

public:
    zlib_filter(
        capy::polystore& ctx,
        http_proto::detail::workspace& ws,
        int window_bits)
        : zlib_filter_base(ws)
        , svc_(ctx.get<capy::zlib::inflate_service>())
    {
        system::error_code ec = static_cast<capy::zlib::error>(
            svc_.init2(strm_, window_bits));
        if(ec != capy::zlib::error::ok)
            detail::throw_system_error(ec);
    }

private:
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
            svc_.inflate(
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

} // http_proto
} // boost
