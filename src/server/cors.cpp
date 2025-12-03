//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/server/cors.hpp>
#include <utility>

namespace boost {
namespace http_proto {

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
    Vary(route_params& p)
        : p_(p)
    {
    }

    void set(field f, core::string_view s)
    {
        p_.res.set(f, s);
    }

    void append(field f, core::string_view v)
    {
        auto it = p_.res.find(f);
        if (it != p_.res.end())
        {
            std::string s = it->value;
            s += ", ";
            s += v;
            p_.res.set(it, s);
        }
        else
        {
            p_.res.set(f, v);
        }
    }

private:
    route_params& p_;
    std::string v_;
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
    route_params const& p,
    cors_options const& options)
{
    if(! options.allowedHeaders.empty())
    {
        v.set(
            field::access_control_allow_headers,
            options.allowedHeaders);
        return;
    }
    auto s = p.res.value_or(
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

route_result
cors::
operator()(
    route_params& p) const
{
    Vary v(p);
    if(p.req.method() ==
        method::options)
    {
        // preflight
        setOrigin(v, p, options_);
        setMethods(v, options_);
        setCredentials(v, options_);
        setAllowedHeaders(v, p, options_);
        setMaxAge(v, options_);
        setExposeHeaders(v, options_);

        if(options_.preFligthContinue)
            return route::next;
        // Safari and others need this for 204 or may hang
        p.res.set_status(options_.result);
        p.res.set_content_length(0);
        p.serializer.start(p.res);
        return route::send;
    }
    // actual response
    setOrigin(v, p, options_);
    setCredentials(v, options_);
    setExposeHeaders(v, options_);
    return route::next;
}

} // http_proto
} // boost
