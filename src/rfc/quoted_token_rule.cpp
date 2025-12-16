//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/rfc/quoted_token_rule.hpp>
#include <boost/http_proto/rfc/token_rule.hpp>
#include <boost/url/grammar/charset.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/lut_chars.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/vchars.hpp>

namespace boost {
namespace http_proto {

namespace {

struct obs_text
{
    constexpr
    bool
    operator()(char ch) const noexcept
    {
        return static_cast<
            unsigned char>(ch) >= 0x80;
    }
};

struct qdtext
{
    constexpr
    bool
    operator()(char ch) const noexcept
    {
        return
            ch == '\t' ||
            ch == ' ' ||
            ch == 0x21 ||
            (ch >= 0x23 && ch <= 0x5b) ||
            (ch >= 0x5d && ch <= 0x7e) ||
            static_cast<unsigned char>(ch) >= 0x80;
    }
};

// qdtext  = HTAB / SP /%x21 / %x23-5B / %x5D-7E / obs-text
constexpr grammar::lut_chars qdtext_chars(qdtext{});

// qpchars = ( HTAB / SP / VCHAR / obs-text )
constexpr auto qpchars =
    grammar::lut_chars(grammar::vchars) +
    grammar::lut_chars(obs_text{}) + '\t' + ' ';

} // namespace


namespace implementation_defined {
auto
quoted_token_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    if(it == end)
    {
        BOOST_HTTP_PROTO_RETURN_EC(
            grammar::error::need_more);
    }
    if(*it != '\"')
    {
        // token
        auto rv = grammar::parse(
            it, end, token_rule);
        if(rv.has_value())
            return quoted_token_view(*rv);
        return rv.error();
    }
    // quoted-string
    auto const it0 = ++it;
    std::size_t n = 0;
    for(;;)
    {
        auto it1 = it;
        it = grammar::find_if_not(
            it, end, qdtext_chars);
        if(it == end)
        {
            BOOST_HTTP_PROTO_RETURN_EC(
                grammar::error::need_more);
        }
        n += static_cast<std::size_t>(it - it1);
        if(*it == '\"')
            break;
        if(*it != '\\')
        {
            BOOST_HTTP_PROTO_RETURN_EC(
                grammar::error::syntax);
        }
        ++it;
        if(it == end)
        {
            BOOST_HTTP_PROTO_RETURN_EC(
                grammar::error::need_more);
        }
        if(! qpchars(*it))
        {
            BOOST_HTTP_PROTO_RETURN_EC(
                grammar::error::syntax);
        }
        ++it;
        ++n;
    }
    return value_type(core::string_view(
        it0, it++ - it0), n);
}

} // implementation_defined
} // http_proto
} // boost
