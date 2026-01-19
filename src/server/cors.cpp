//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/cors.hpp>
#include <utility>

namespace boost {
namespace http {

cors::
cors(
    cors_options options) noexcept
    : options_(std::move(options))
{
    // VFALCO TODO Validate the strings in options against RFC
}

namespace {

struct Vary
{
    Vary(route_params& rp)
        : rp_(rp)
    {
    }

    void set(field f, core::string_view s)
    {
        rp_.res.set(f, s);
    }

    void append(field f, core::string_view v)
    {
        auto it = rp_.res.find(f);
        if(it != rp_.res.end())
        {
            std::string s = it->value;
            s += ", ";
            s += v;
            rp_.res.set(it, s);
        }
        else
        {
            rp_.res.set(f, v);
        }
    }

private:
    route_params& rp_;
};

} // (anon)

// Access-Control-Allow-Origin
static void setOrigin(
    Vary& v,
    route_params const&,
    cors_options const& options)
{
    if( options.origin.empty() ||
        options.origin == "*")
    {
        v.set(field::access_control_allow_origin, "*");
        return;
    }

    v.set(
        field::access_control_allow_origin,
        options.origin);
    v.append(field::vary, to_string(field::origin));
}

// Access-Control-Allow-Methods
static void setMethods(
    Vary& v,
    cors_options const& options)
{
    if(! options.methods.empty())
    {
        v.set(
            field::access_control_allow_methods,
            options.methods);
        return;
    }
    v.set(
        field::access_control_allow_methods,
        "GET,HEAD,PUT,PATCH,POST,DELETE");
}

// Access-Control-Allow-Credentials
static void setCredentials(
    Vary& v,
    cors_options const& options)
{
    if(! options.credentials)
        return;
    v.set(
        field::access_control_allow_credentials,
        "true");
}

// Access-Control-Allowed-Headers
static void setAllowedHeaders(
    Vary& v,
    route_params const& rp,
    cors_options const& options)
{
    if(! options.allowedHeaders.empty())
    {
        v.set(
            field::access_control_allow_headers,
            options.allowedHeaders);
        return;
    }
    auto s = rp.req.value_or(
        field::access_control_request_headers, "");
    if(! s.empty())
    {
        v.set(field::access_control_allow_headers, s);
        v.append(field::vary, s);
    }
}

// Access-Control-Expose-Headers
static void setExposeHeaders(
    Vary& v,
    cors_options const& options)
{
    if(options.exposedHeaders.empty())
        return;
    v.set(
        field::access_control_expose_headers,
        options.exposedHeaders);
}

// Access-Control-Max-Age
static void setMaxAge(
    Vary& v,
    cors_options const& options)
{
    if(options.max_age.count() == 0)
        return;
    v.set(
        field::access_control_max_age,
        std::to_string(
            options.max_age.count()));
}

route_task
cors::
operator()(
    route_params& rp) const
{
    Vary v(rp);
    if(rp.req.method() == method::options)
    {
        // preflight
        setOrigin(v, rp, options_);
        setMethods(v, options_);
        setCredentials(v, options_);
        setAllowedHeaders(v, rp, options_);
        setMaxAge(v, options_);
        setExposeHeaders(v, options_);

        if(options_.preFlightContinue)
            co_return route::next;

        // Safari and others need this for 204 or may hang
        rp.res.set_status(options_.result);
        co_return co_await rp.send("");
    }

    // actual response
    setOrigin(v, rp, options_);
    setCredentials(v, options_);
    setExposeHeaders(v, options_);
    co_return route::next;
}

} // http
} // boost
