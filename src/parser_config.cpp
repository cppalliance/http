//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/parser_config.hpp>
#include <boost/http_proto/detail/except.hpp>
#include <boost/http_proto/detail/header.hpp>
#include <boost/http_proto/detail/workspace.hpp>
#include <src/zlib_filter.hpp>
#include <boost/capy/brotli/encode.hpp>
#include <boost/capy/brotli/decode.hpp>
#include <boost/capy/zlib/deflate.hpp>
#include <boost/capy/zlib/inflate.hpp>

namespace boost {
namespace http_proto {

parser_config::
parser_config(
    role role,
    capy::polystore& ps)
    : ps_(ps)
{
    if(role == http_proto::role::client)
    {
        body_limit = 64 * 1024;
    }
    else
    {
        body_limit = 64 * 1024;
    }
}

void
parser_config::
prepare(
    std::shared_ptr<parser_config> sp)
{
    sp->ps_.find(sp->brotli_decode);
    sp->ps_.find(sp->brotli_encode);
    sp->ps_.find(sp->zlib_deflate);
    sp->ps_.find(sp->zlib_inflate);

    /*
        | fb |     cb0     |     cb1     | C | T | f |

        fb  flat_buffer         headers.max_size
        cb0 circular_buffer     min_buffer
        cb1 circular_buffer     min_buffer
        C   codec               max_codec
        T   body                max_type_erase
        f   table               max_table_space

    */
    // validate
    //if(sp->min_prepare > sp->max_prepare)
        //detail::throw_invalid_argument();

    if(sp->max_prepare < 1)
        detail::throw_invalid_argument();

    // VFALCO TODO OVERFLOW CHECING
    {
        //fb_.size() - h_.size +
        //svc_.sp->min_buffer +
        //svc_.sp->min_buffer +
        //svc_.max_codec;
    }

    // VFALCO OVERFLOW CHECKING ON THIS
    sp->space_needed +=
        sp->headers.valid_space_needed();

    // cb0_, cb1_
    // VFALCO OVERFLOW CHECKING ON THIS
    sp->space_needed +=
        sp->min_buffer +
        sp->min_buffer;

    // T
    sp->space_needed += sp->max_type_erase;

    // max_codec
    if(sp->apply_deflate_decoder || sp->apply_gzip_decoder)
    {
        // TODO: Account for the number of allocations and
        // their overhead in the workspace.

        // https://www.zlib.net/zlib_tech.html
        std::size_t n =
            (1 << sp->zlib_window_bits) +
            (7 * 1024) +
            #ifdef __s390x__
            5768 +
            #endif
            detail::workspace::space_needed<
                zlib_filter>();

        if( sp->max_codec < n)
            sp->max_codec = n;
    }
    sp->space_needed += sp->max_codec;

    // round up to alignof(detail::header::entry)
    auto const al = alignof(
        detail::header::entry);
    sp->space_needed = al * ((
        sp->space_needed + al - 1) / al);
}

} // http_proto
} // boost
