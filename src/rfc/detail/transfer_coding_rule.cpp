//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/rfc/detail/transfer_coding_rule.hpp"

#include <boost/http/rfc/detail/ws.hpp>
#include <boost/http/rfc/quoted_token_rule.hpp>
#include <boost/http/rfc/token_rule.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/url/grammar/parse.hpp>

namespace boost {
namespace http {
namespace detail {

auto
transfer_parameter_rule_t::parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    value_type t;
    auto it0 = it;
    // OWS
    it = grammar::find_if_not(
        it, end, ws);
    // ";"
    if(it == end)
    {
        it = it0;
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    if(*it != ';')
    {
        it = it0;
        BOOST_HTTP_RETURN_EC(
            grammar::error::mismatch);
    }
    ++it;
    // OWS
    it = grammar::find_if_not(
        it, end, ws);
    // token
    {
        auto rv = grammar::parse(
            it, end, token_rule);
        if(! rv)
            return rv.error();
        t.name = *rv;
    }
    // BWS
    it = grammar::find_if_not(
        it, end, ws);
    // "="
    if(it == end)
    {
        it = it0;
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    if(*it != '=')
    {
        it = it0;
        BOOST_HTTP_RETURN_EC(
            grammar::error::syntax);
    }
    ++it;
    // BWS
    it = grammar::find_if_not(
        it, end, ws);
    // quoted-token
    {
        auto rv = grammar::parse(
            it, end, quoted_token_rule);
        if(! rv)
            return rv.error();
        t.value = *rv;
    }
    return t;
}

//------------------------------------------------

auto
transfer_coding_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    value_type t;
    {
        // token
        auto rv = grammar::parse(
            it, end, token_rule);
        if(! rv)
            return rv.error();

        // These can't have transfer-parameter
        if(grammar::ci_is_equal(
            *rv, "chunked"))
        {
            t.id = coding::chunked;
            return t;
        }
        if(grammar::ci_is_equal(
            *rv, "compress"))
        {
            t.id = coding::compress;
            return t;
        }
        if(grammar::ci_is_equal(
            *rv, "deflate"))
        {
            t.id = coding::deflate;
            return t;
        }
        if(grammar::ci_is_equal(
            *rv, "gzip"))
        {
            t.id = coding::gzip;
            return t;
        }

        t.extension.token = *rv;
    }
    // transfer-extension = token *( OWS ";" OWS transfer-parameter )
    {
        auto rv = grammar::parse(it, end,
            grammar::range_rule(
                detail::transfer_parameter_rule));
        if(! rv)
            return rv.error();
        t.extension.params = std::move(*rv);
    }
    return t;
}

} // detail
} // http
} // boost
