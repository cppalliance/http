//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/method.hpp>
#include <boost/http/detail/sv.hpp>
#include <boost/throw_exception.hpp>
#include <ostream>

namespace boost {
namespace http {

core::string_view
to_string(method v)
{
    using namespace detail::string_literals;
    switch(v)
    {
    case method::delete_:       return "DELETE"_sv;
    case method::get:           return "GET"_sv;
    case method::head:          return "HEAD"_sv;
    case method::post:          return "POST"_sv;
    case method::put:           return "PUT"_sv;
    case method::connect:       return "CONNECT"_sv;
    case method::options:       return "OPTIONS"_sv;
    case method::trace:         return "TRACE"_sv;

    case method::copy:          return "COPY"_sv;
    case method::lock:          return "LOCK"_sv;
    case method::mkcol:         return "MKCOL"_sv;
    case method::move:          return "MOVE"_sv;
    case method::propfind:      return "PROPFIND"_sv;
    case method::proppatch:     return "PROPPATCH"_sv;
    case method::search:        return "SEARCH"_sv;
    case method::unlock:        return "UNLOCK"_sv;
    case method::bind:          return "BIND"_sv;
    case method::rebind:        return "REBIND"_sv;
    case method::unbind:        return "UNBIND"_sv;
    case method::acl:           return "ACL"_sv;

    case method::report:        return "REPORT"_sv;
    case method::mkactivity:    return "MKACTIVITY"_sv;
    case method::checkout:      return "CHECKOUT"_sv;
    case method::merge:         return "MERGE"_sv;

    case method::msearch:       return "M-SEARCH"_sv;
    case method::notify:        return "NOTIFY"_sv;
    case method::subscribe:     return "SUBSCRIBE"_sv;
    case method::unsubscribe:   return "UNSUBSCRIBE"_sv;

    case method::patch:         return "PATCH"_sv;
    case method::purge:         return "PURGE"_sv;

    case method::mkcalendar:    return "MKCALENDAR"_sv;

    case method::link:          return "LINK"_sv;
    case method::unlink:        return "UNLINK"_sv;

    case method::unknown:
        return "<unknown>"_sv;
    }

    BOOST_THROW_EXCEPTION(
        std::invalid_argument("unknown method"));
}

method
string_to_method(
    core::string_view v)
{
/*
    ACL
    BIND
    CHECKOUT
    CONNECT
    COPY
    DELETE
    GET
    HEAD
    LINK
    LOCK
    M-SEARCH
    MERGE
    MKACTIVITY
    MKCALENDAR
    MKCOL
    MOVE
    NOTIFY
    OPTIONS
    PATCH
    POST
    PROPFIND
    PROPPATCH
    PURGE
    PUT
    REBIND
    REPORT
    SEARCH
    SUBSCRIBE
    TRACE
    UNBIND
    UNLINK
    UNLOCK
    UNSUBSCRIBE
*/
    using namespace detail::string_literals;
    if(v.size() < 3)
        return method::unknown;
    auto c = v[0];
    v.remove_prefix(1);
    switch(c)
    {
    case 'A':
        if(v == "CL"_sv)
            return method::acl;
        break;

    case 'B':
        if(v == "IND"_sv)
            return method::bind;
        break;

    case 'C':
        c = v[0];
        v.remove_prefix(1);
        switch(c)
        {
        case 'H':
            if(v == "ECKOUT"_sv)
                return method::checkout;
            break;

        case 'O':
            if(v == "NNECT"_sv)
                return method::connect;
            if(v == "PY"_sv)
                return method::copy;
            BOOST_FALLTHROUGH;

        default:
            break;
        }
        break;

    case 'D':
        if(v == "ELETE"_sv)
            return method::delete_;
        break;

    case 'G':
        if(v == "ET"_sv)
            return method::get;
        break;

    case 'H':
        if(v == "EAD"_sv)
            return method::head;
        break;

    case 'L':
        if(v == "INK"_sv)
            return method::link;
        if(v == "OCK"_sv)
            return method::lock;
        break;

    case 'M':
        c = v[0];
        v.remove_prefix(1);
        switch(c)
        {
        case '-':
            if(v == "SEARCH"_sv)
                return method::msearch;
            break;

        case 'E':
            if(v == "RGE"_sv)
                return method::merge;
            break;

        case 'K':
            if(v == "ACTIVITY"_sv)
                return method::mkactivity;
            if(v[0] == 'C')
            {
                v.remove_prefix(1);
                if(v == "ALENDAR"_sv)
                    return method::mkcalendar;
                if(v == "OL"_sv)
                    return method::mkcol;
                break;
            }
            break;

        case 'O':
            if(v == "VE"_sv)
                return method::move;
            BOOST_FALLTHROUGH;

        default:
            break;
        }
        break;

    case 'N':
        if(v == "OTIFY"_sv)
            return method::notify;
        break;

    case 'O':
        if(v == "PTIONS"_sv)
            return method::options;
        break;

    case 'P':
        c = v[0];
        v.remove_prefix(1);
        switch(c)
        {
        case 'A':
            if(v == "TCH"_sv)
                return method::patch;
            break;

        case 'O':
            if(v == "ST"_sv)
                return method::post;
            break;

        case 'R':
            if(v == "OPFIND"_sv)
                return method::propfind;
            if(v == "OPPATCH"_sv)
                return method::proppatch;
            break;

        case 'U':
            if(v == "RGE"_sv)
                return method::purge;
            if(v == "T"_sv)
                return method::put;
            BOOST_FALLTHROUGH;

        default:
            break;
        }
        break;

    case 'R':
        if(v[0] != 'E')
            break;
        v.remove_prefix(1);
        if(v == "BIND"_sv)
            return method::rebind;
        if(v == "PORT"_sv)
            return method::report;
        break;

    case 'S':
        if(v == "EARCH"_sv)
            return method::search;
        if(v == "UBSCRIBE"_sv)
            return method::subscribe;
        break;

    case 'T':
        if(v == "RACE"_sv)
            return method::trace;
        break;

    case 'U':
        if(v[0] != 'N')
            break;
        v.remove_prefix(1);
        if(v == "BIND"_sv)
            return method::unbind;
        if(v == "LINK"_sv)
            return method::unlink;
        if(v == "LOCK"_sv)
            return method::unlock;
        if(v == "SUBSCRIBE"_sv)
            return method::unsubscribe;
        break;

    default:
        break;
    }

    return method::unknown;
}

std::ostream&
operator<<(
    std::ostream& os,
    method v)
{
    os << to_string(v);
    return os;
}

} // http
} // boost
