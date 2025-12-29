//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/server/route_handler.hpp>
#include <boost/http_proto/string_body.hpp>

namespace boost {
namespace http_proto {

route_params::
~route_params()
{
}

route_params&
route_params::
status(
    http_proto::status code)
{
    res.set_start_line(code, res.version());
    return *this;
}

route_params&
route_params::
set_body(std::string s)
{
    if(! res.exists(http_proto::field::content_type))
    {
        // VFALCO TODO detect Content-Type
        res.set(http_proto::field::content_type,
            "text/plain; charset=UTF-8");
    }

    if(! res.exists(field::content_length))
    {
        res.set_payload_size(s.size());
    }

    serializer.start(res,
        http_proto::string_body(std::string(s)));

    return *this;
}

#ifdef BOOST_HTTP_PROTO_HAS_CORO

auto
route_params::
spawn(
    capy::task<route_result> t) ->
        route_result
{
    return this->suspend(
        [ex = this->ex, t = std::move(t)](resumer resume) mutable
        {
            auto h = t.release();

            h.promise().on_done = [resume, h]()
            {
                auto& r = h.promise().result;
                if(r.index() == 2)
                {
                    auto ep = std::get<2>(r);
                    h.destroy();
                    resume(ep);
                    return;
                }
                auto rv = std::move(std::get<1>(r));
                h.destroy();
                resume(rv);
            };

            ex.post([h]() { h.resume(); });
        });
}

#endif

} // http_proto
} // boost
