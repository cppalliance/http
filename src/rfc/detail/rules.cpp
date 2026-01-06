//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/rfc/detail/rules.hpp"

#include <boost/http/error.hpp>
#include <boost/http/detail/config.hpp>
#include <boost/http/rfc/token_rule.hpp>

#include <boost/core/detail/string_view.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/digit_chars.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include <boost/url/grammar/lut_chars.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>

#include "src/rfc/detail/rules.hpp"

namespace boost {
namespace http {
namespace detail {

auto
crlf_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    if(it == end)
        return grammar::error::need_more;
    if(*it != '\r')
        return grammar::error::mismatch;
    ++it;
    if(it == end)
        return grammar::error::need_more;
    if(*it != '\n')
        return grammar::error::mismatch;
    ++it;
    return {};
}

//------------------------------------------------

auto
version_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    value_type v = 0;
    if(it == end)
    {
        // expected "HTTP/"
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    if(end - it >= 5)
    {
        if(std::memcmp(
            it, "HTTP/", 5) != 0)
        {
            BOOST_HTTP_RETURN_EC(
                grammar::error::mismatch);
        }
        it += 5;
    }
    if(it == end)
    {
        // expected DIGIT
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    if(! grammar::digit_chars(*it))
    {
        // expected DIGIT
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    v = 10 * (*it++ - '0');
    if(it == end)
    {
        // expected "."
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    if(*it != '.')
    {
        // expected "."
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    ++it;
    if(it == end)
    {
        // expected DIGIT
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    if(! grammar::digit_chars(*it))
    {
        // expected DIGIT
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    v += *it++ - '0';
    return v;
}

//------------------------------------------------

auto
status_code_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    auto const dig =
        [](char c) -> int
        {
            unsigned char uc(c - '0');
            if(uc > 9)
                return -1;
            return uc;
        };

    if(it == end)
    {
        // end
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    auto it0 = it;
    int v = dig(*it);
    if(v == -1)
    {
        // expected DIGIT
        BOOST_HTTP_RETURN_EC(
            grammar::error::mismatch);
    }
    value_type t;
    t.v = 100 * v;
    ++it;
    if(it == end)
    {
        // end
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    v = dig(*it);
    if(v == -1)
    {
        // expected DIGIT
        BOOST_HTTP_RETURN_EC(
            grammar::error::mismatch);
    }
    t.v = t.v + (10 * v);
    ++it;
    if(it == end)
    {
        // end
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    v = dig(*it);
    if(v == -1)
    {
        // expected DIGIT
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    t.v = t.v + v;
    ++it;

    t.s = core::string_view(it0, it - it0);
    t.st = int_to_status(t.v);
    return t;
}

//------------------------------------------------

auto
reason_phrase_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    auto begin = it;
    it = grammar::find_if_not(it, end, ws_vchars);
    return core::string_view(begin, it);
}

//------------------------------------------------

auto
field_name_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    if( it == end )
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);

    value_type v;

    auto begin = it;
    auto rv = grammar::parse(
        it, end, token_rule);
    if( rv.has_error() || (it != end) )
    {
        if( it != begin )
        {
            v = core::string_view(begin, it - begin);
            return v;
        }
        return error::bad_field_name;
    }

    v = core::string_view(begin, end - begin);
    return v;
}

auto
field_value_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    value_type v;
    if( it == end )
    {
        v.value = core::string_view(it, 0);
        return v;
    }

    // field-line     = field-name ":" OWS field-value OWS
    // field-value    = *field-content
    // field-content  = field-vchar
    //                  [ 1*( SP / HTAB / field-vchar ) field-vchar ]
    // field-vchar    = VCHAR / obs-text
    // obs-text       = %x80-FF
    // VCHAR          = %x21-7E
    //                       ; visible (printing) characters

    auto is_field_vchar = [](unsigned char ch)
    {
      return (ch >= 0x21 && ch <= 0x7e) || ch >= 0x80;
    };

    char const* s0 = nullptr;
    char const* s1 = nullptr;

    bool has_crlf = false;
    bool has_obs_fold = false;

    while( it < end )
    {
        auto ch = *it;
        if( ws(ch) )
        {
            ++it;
            continue;
        }

        if( ch == '\r' )
        {
            // too short to know if we have a potential obs-fold
            // occurrence
            if( end - it < 2 )
                BOOST_HTTP_RETURN_EC(
                    grammar::error::need_more);

            if( it[1] != '\n' )
                goto done;

            if( end - it < 3 )
                BOOST_HTTP_RETURN_EC(
                    grammar::error::need_more);

            if(! ws(it[2]) )
            {
                has_crlf = true;
                goto done;
            }

            has_obs_fold = true;
            it = it + 3;
            continue;
        }

        if(! is_field_vchar(ch) )
        {
            goto done;
        }

        if(! s0 )
            s0 = it;

        ++it;
        s1 = it;
    }

done:
    // later routines wind up doing pointer
    // subtraction using the .data() member
    // of the value so we need a valid 0-len range
    if(! s0 )
    {
        s0 = it;
        s1 = s0;
    }

    v.value = core::string_view(s0, s1 - s0);
    v.has_crlf = has_crlf;
    v.has_obs_fold = has_obs_fold;
    return v;
}

auto
field_rule_t::
parse(
    char const*& it,
    char const* end) const noexcept ->
        system::result<value_type>
{
    if(it == end)
    {
        BOOST_HTTP_RETURN_EC(
            grammar::error::need_more);
    }
    // check for leading CRLF
    if(it[0] == '\r')
    {
        ++it;
        if(it == end)
        {
            BOOST_HTTP_RETURN_EC(
                grammar::error::need_more);
        }
        if(*it != '\n')
        {
            BOOST_HTTP_RETURN_EC(
                grammar::error::mismatch);
        }
        // end of fields
        ++it;
        BOOST_HTTP_RETURN_EC(
            grammar::error::end_of_range);
    }

    value_type v;
    auto rv = grammar::parse(
        it, end, grammar::tuple_rule(
            field_name_rule,
            grammar::delim_rule(':'),
            field_value_rule,
            crlf_rule));

    if( rv.has_error() )
        return rv.error();

    auto val = rv.value();
    v.name = std::get<0>(val);
    v.value = std::get<2>(val).value;
    v.has_obs_fold = std::get<2>(val).has_obs_fold;

    return v;
}

//------------------------------------------------

void
remove_obs_fold(
    char* it,
    char const* const end) noexcept
{
    while(it != end)
    {
        if(*it != '\r')
        {
            ++it;
            continue;
        }
        if(end - it < 3)
            break;
        BOOST_ASSERT(it[1] == '\n');
        if( it[1] == '\n' &&
            ws(it[2]))
        {
            it[0] = ' ';
            it[1] = ' ';
            it += 3;
        }
        else
        {
            ++it;
        }
    }
}

} // detail
} // http
} // boost
