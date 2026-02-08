//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/json/json_body.hpp>
#include <boost/http/json/json_sink.hpp>
#include <boost/http/field.hpp>
#include <boost/http/method.hpp>
#include <boost/capy/io/push_to.hpp>
#include <boost/json/value.hpp>

namespace boost {
namespace http {

json_body::
json_body(json_body_options options) noexcept
    : options_(std::move(options))
{
}

route_task
json_body::
operator()(route_params& p) const
{
    if( ! p.is_method(method::post) ||
        ! p.req.value_or(
            field::content_type, "")
                .starts_with("application/json"))
        co_return route_next;

    json_sink sink(
        options_.storage, options_.parse_opts);
    auto [ec, n] = co_await capy::push_to(
        p.req_body, sink);
    if(ec)
        co_return route_error(ec);

    p.route_data.emplace<json::value>(
        sink.release());

    co_return route_next;
}

} // namespace http
} // namespace boost
