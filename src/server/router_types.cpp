//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/router_types.hpp>
#include <boost/http/server/router.hpp>
#include <boost/http/server/etag.hpp>
#include <boost/http/server/fresh.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/assert.hpp>
#include <cstring>

namespace boost {
namespace system {
template<>
struct is_error_code_enum<
    ::boost::http::route_what>
{
    static bool const value = true;
};
} // system
} // boost

namespace boost {
namespace http {

namespace {

struct route_what_cat_type
    : system::error_category
{
    constexpr route_what_cat_type()
        : error_category(0x7a8b3c4d5e6f1029)
    {
    }

    const char* name() const noexcept override
    {
        return "boost.http.route_what";
    }

    std::string message(int code) const override
    {
        return message(code, nullptr, 0);
    }

    char const* message(
        int code,
        char*,
        std::size_t) const noexcept override
    {
        switch(static_cast<route_what>(code))
        {
        case route_what::done:          return "done";
        case route_what::error:         return "error";
        case route_what::next:          return "next";
        case route_what::next_route:    return "next_route";
        case route_what::close:         return "close";
        default:
            return "?";
        }
    }
};

route_what_cat_type route_what_cat;

} // (anon)

system::error_code
make_error_code(
    route_what w) noexcept
{
    return system::error_code{
        static_cast<int>(w), route_what_cat};
}

//----------------------------------------------------------

route_result::
route_result(
    system::error_code ec)
    : ec_(ec)
{
    if(! ec.failed())
        detail::throw_invalid_argument();
}

void
route_result::
set(route_what w)
{
    ec_ = make_error_code(w);
}

auto
route_result::
what() const noexcept ->
    route_what
{
    if(! ec_.failed())
        return route_what::done;    
    if(&ec_.category() != &route_what_cat)
        return route_what::error;
    if( ec_ == route_what::next)
        return route_what::next;
    if( ec_ == route_what::next_route)
        return route_what::next_route;
    //if( ec_ == route_what::close)
    return route_what::close;
}

auto
route_result::
error() const noexcept ->
    system::error_code
{
    if(&ec_.category() != &route_what_cat)
        return ec_;
    return {};
}

//------------------------------------------------

bool
route_params_base::
is_method(
    core::string_view s) const noexcept
{
    auto m = http::string_to_method(s);
    if(m != http::method::unknown)
        return verb_ == m;
    return s == verb_str_;
}

//------------------------------------------------

capy::io_task<>
route_params::
send(std::string_view body)
{
    auto const sc = res.status();

    // 204 No Content / 304 Not Modified: strip headers, no body
    if(sc == status::no_content ||
       sc == status::not_modified)
    {
        res.erase(field::content_type);
        res.erase(field::content_length);
        res.erase(field::transfer_encoding);
        co_return co_await res_body.write_eof();
    }

    // 205 Reset Content: Content-Length=0, no body
    if(sc == status::reset_content)
    {
        res.erase(field::transfer_encoding);
        res.set_payload_size(0);
        co_return co_await res_body.write_eof();
    }

    // Set Content-Type if not already set
    if(! res.exists(field::content_type))
    {
        if(! body.empty() && body[0] == '<')
            res.set(field::content_type,
                "text/html; charset=utf-8");
        else
            res.set(field::content_type,
                "text/plain; charset=utf-8");
    }

    // Generate ETag if not already set
    if(! res.exists(field::etag))
        res.set(field::etag, etag(body));

    // Set Content-Length if not already set
    if(! res.exists(field::content_length))
        res.set_payload_size(body.size());

    // Freshness check: auto-304 for conditional GET
    if(is_fresh(req, res))
    {
        res.set_status(status::not_modified);
        res.erase(field::content_type);
        res.erase(field::content_length);
        res.erase(field::transfer_encoding);
        co_return co_await res_body.write_eof();
    }

    // HEAD: send headers only, skip body
    if(req.method() == method::head)
    {
        co_return co_await res_body.write_eof();
    }

    auto [ec, n] = co_await res_body.write(
        capy::make_buffer(body), true);
    co_return {ec};
}

} // http
} // boost
