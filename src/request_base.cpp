//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Christian Mazakas
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/request_base.hpp>

#include <cstring>

namespace boost {
namespace http {

void
request_base::
set_expect_100_continue(bool b)
{
    if(h_.md.expect.count == 0)
    {
        BOOST_ASSERT(
            ! h_.md.expect.ec);
        BOOST_ASSERT(
            ! h_.md.expect.is_100_continue);
        if( b )
        {
            append(
                field::expect,
                "100-continue");
            return;
        }
        return;
    }

    if(h_.md.expect.count == 1)
    {
        if(b)
        {
            if(! h_.md.expect.ec)
            {
                BOOST_ASSERT(
                    h_.md.expect.is_100_continue);
                return;
            }
            BOOST_ASSERT(
                ! h_.md.expect.is_100_continue);
            auto it = find(field::expect);
            BOOST_ASSERT(it != end());
            set(it, "100-continue");
            return;
        }

        auto it = find(field::expect);
        BOOST_ASSERT(it != end());
        erase(it);
        return;
    }

    BOOST_ASSERT(h_.md.expect.ec);

    auto nc = (b ? 1 : 0);
    auto ne = h_.md.expect.count - nc;
    if( b )
        set(find(field::expect), "100-continue");

    raw_erase_n(field::expect, ne);
    h_.md.expect.count = nc;
    h_.md.expect.ec = {};
    h_.md.expect.is_100_continue = b;
}

//------------------------------------------------

void
request_base::
set_start_line_impl(
    http::method m,
    core::string_view ms,
    core::string_view t,
    http::version v)
{
    // TODO: check validity
    auto const vs = to_string(v);
    auto const new_prefix =
        ms.size() + 1 + // method SP
        t.size() + 1 +  // request-target SP
        vs.size() + 2;  // HTTP-version CRLF

    // Introduce a new scope so that prefix_op's
    // destructor runs before h_.on_start_line().
    {
        auto op = prefix_op_t(
            *this, new_prefix, &ms, &t);

        h_.version = v;
        h_.req.method = m;
        h_.req.method_len = static_cast<
            offset_type>(ms.size());
        h_.req.target_len = static_cast<
            offset_type>(t.size());

        char* m_dest = h_.buf;
        char* t_dest = h_.buf + ms.size() + 1;
        char* v_dest = t_dest + t.size() + 1;

        std::memmove(t_dest, t.data(), t.size());
        t_dest[t.size()] = ' ';

        // memmove after target because could overlap
        std::memmove(m_dest, ms.data(), ms.size());
        m_dest[ms.size()] = ' ';

        std::memcpy(v_dest, vs.data(), vs.size());
        v_dest[vs.size() + 0] = '\r';
        v_dest[vs.size() + 1] = '\n';
    }

    h_.on_start_line();
}

} // http
} // boost
