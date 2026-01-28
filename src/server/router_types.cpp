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
#include <boost/url/grammar/ci_string.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/assert.hpp>
#include <cstring>

namespace boost {
namespace http {

namespace detail {

const char*
route_cat_type::
name() const noexcept
{
    return "boost.http.route";
}

std::string
route_cat_type::
message(int code) const
{
    return message(code, nullptr, 0);
}

char const*
route_cat_type::
message(
    int code,
    char*,
    std::size_t) const noexcept
{
    switch(static_cast<route>(code))
    {
    case route::next:       return "next";
    case route::next_route: return "next_route";
    case route::close:      return "close";
    default:
        return "?";
    }
}

// msvc 14.0 has a bug that warns about inability
// to use constexpr construction here, even though
// there's no constexpr construction
#if defined(_MSC_VER) && _MSC_VER <= 1900
# pragma warning( push )
# pragma warning( disable : 4592 )
#endif

#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
constinit route_cat_type route_cat;
#else
route_cat_type route_cat;
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1900
# pragma warning( pop )
#endif

} // detail

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
