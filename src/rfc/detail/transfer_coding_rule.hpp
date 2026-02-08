//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_TRANSFER_CODING_RULE_HPP
#define BOOST_HTTP_RFC_TRANSFER_CODING_RULE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/rfc/quoted_token_view.hpp>
#include <boost/core/detail/string_view.hpp>
#include <boost/url/grammar/range_rule.hpp>

namespace boost {
namespace http {
namespace detail {

/*
    https://datatracker.ietf.org/doc/html/rfc7230#section-4

    transfer-parameter = token BWS "=" BWS ( token / quoted-string )
*/
struct transfer_parameter_rule_t
{
    struct value_type
    {
        core::string_view name;
        quoted_token_view value;
    };

    auto
    parse(
        char const*& it,
        char const* end) const noexcept ->
            system::result<value_type>;
};

constexpr transfer_parameter_rule_t transfer_parameter_rule{};

/*
    https://datatracker.ietf.org/doc/html/rfc7230#section-4

    transfer-coding    = "chunked"
                       / "compress" ;
                       / "deflate" ;
                       / "gzip" ;
                       / transfer-extension
    transfer-extension = token *( OWS ";" OWS transfer-parameter )
*/
struct transfer_coding_rule_t
{
    enum coding
    {
        unknown,
        chunked,
        compress,
        deflate,
        gzip
    };

    struct value_type
    {
        coding id = unknown;
        struct
        {
            core::string_view token;
            grammar::range<
                transfer_parameter_rule_t::
                    value_type> params;
        } extension;
    };

    auto
    parse(
        char const*& it,
        char const* end) const noexcept ->
            system::result<value_type>;
};

constexpr transfer_coding_rule_t transfer_coding_rule{};

} // detail
} // http
} // boost

#endif
