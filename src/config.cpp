//
// Copyright (c) 2026 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/config.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/http/detail/header.hpp>

#include <cstdint>
#include <memory>

namespace boost {
namespace http {

std::size_t
parser_config_impl::
max_overread() const noexcept
{
    return headers.max_size + min_buffer;
}

std::shared_ptr<parser_config_impl const>
make_parser_config(parser_config cfg)
{
    if(cfg.max_prepare < 1)
        detail::throw_invalid_argument();

    auto impl = std::make_shared<parser_config_impl>(std::move(cfg));

    /*
        | fb |     cb0     |     cb1     | T | f |

        fb  flat_dynamic_buffer         headers.max_size
        cb0 circular_buffer     min_buffer
        cb1 circular_buffer     min_buffer
        T   body                max_type_erase
        f   table               max_table_space
    */

    std::size_t space_needed = 0;

    space_needed += impl->headers.valid_space_needed();

    // cb0_, cb1_
    space_needed += impl->min_buffer + impl->min_buffer;

    // T
    space_needed += impl->max_type_erase;

    // round up to alignof(detail::header::entry)
    auto const al = alignof(detail::header::entry);
    space_needed = al * ((space_needed + al - 1) / al);

    impl->space_needed = space_needed;
    impl->max_codec = 0;  // not currently used for parser

    return impl;
}

std::shared_ptr<serializer_config_impl const>
make_serializer_config(serializer_config cfg)
{
    auto impl = std::make_shared<serializer_config_impl>();
    static_cast<serializer_config&>(*impl) = std::move(cfg);

    std::size_t space_needed = 0;
    space_needed += impl->payload_buffer;
    space_needed += impl->max_type_erase;

    impl->space_needed = space_needed;

    return impl;
}

} // http
} // boost
