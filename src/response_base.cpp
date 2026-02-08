//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/response_base.hpp>

#include <cstring>

namespace boost {
namespace http {

void
response_base::
set_start_line_impl(
    http::status sc,
    unsigned short si,
    core::string_view rs,
    http::version v)
{
    // TODO: check validity
    auto const vs = to_string(v);
    auto const new_prefix =
        vs.size() + 1 + // HTTP-version SP
        3 + 1 +         // status-code SP
        rs.size() + 2;  // reason-phrase CRLF

    // Introduce a new scope so that prefix_op's
    // destructor runs before h_.on_start_line().
    {
        auto op = prefix_op_t(*this, new_prefix, &rs);
        char* dest = h_.buf;

        h_.version = v;
        vs.copy(dest, vs.size());
        dest += vs.size();
        *dest++ = ' ';

        h_.res.status = sc;
        h_.res.status_int = si;
        dest[0] = '0' + ((h_.res.status_int / 100) % 10);
        dest[1] = '0' + ((h_.res.status_int /  10) % 10);
        dest[2] = '0' + ((h_.res.status_int /   1) % 10);
        dest[3] = ' ';
        dest += 4;

        std::memmove(dest, rs.data(), rs.size());
        dest += rs.size();
        dest[0] = '\r';
        dest[1] = '\n';
    }

    h_.on_start_line();
}

void
response_base::
set_version(
    http::version v)
{
    if(v == h_.version)
        return;
    if(h_.is_default())
    {
        auto def = h_.get_default(detail::kind::response);
        return set_start_line_impl(
            def->res.status, def->res.status_int,
            core::string_view(
                def->cbuf + 13, def->prefix - 15), v);
    }

    // Introduce a new scope so that prefix_op's
    // destructor runs before h_.on_start_line().
    {
        auto op = prefix_op_t(
            *this, h_.prefix, nullptr);
        char* dest = h_.buf;
        if(v == http::version::http_1_1)
            dest[7] = '1';
        else
            dest[7] = '0';
        h_.version = v;
    }

    h_.on_start_line();
}

} // http
} // boost
