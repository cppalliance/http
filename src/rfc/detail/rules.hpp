//
// Copyright (c) 2024 Christian Mazakas
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SRC_RFC_DETAIL_RULES_HPP
#define BOOST_HTTP_SRC_RFC_DETAIL_RULES_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/rfc/detail/ws.hpp>
#include <boost/http/rfc/token_rule.hpp>
#include <boost/http/status.hpp>
#include <boost/core/detail/string_view.hpp>
#include <boost/system/result.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/lut_chars.hpp>
#include <boost/url/grammar/token_rule.hpp>
#include <boost/url/grammar/tuple_rule.hpp>

namespace boost {
namespace http {
namespace detail {

//------------------------------------------------

/*  Used with list_rule

    @par BNF
    @code
    ows-comma   = OWS "," OWS
    @endcode
*/
struct ows_comma_ows_rule_t
{
    using value_type = void;

    auto
    parse(
        char const*& it,
        char const* end) const noexcept ->
            system::result<void>
    {
        // OWS
        it = grammar::find_if_not(
            it, end, ws);
        if(it == end)
            return grammar::error::mismatch;
        // ","
        if(*it != ',')
            return grammar::error::mismatch;
        ++it;
        // OWS
        it = grammar::find_if_not(
            it, end, ws);
        return {};
    }
};

constexpr ows_comma_ows_rule_t ows_comma_ows_rule{};

//------------------------------------------------

// used for request-target
//
// target-char    = <any OCTET except CTLs, and excluding LWS>
//
struct target_chars_t
{
    constexpr
    bool
    operator()(char c) const noexcept
    {
        return
            (static_cast<unsigned char>(c) >= 0x21) &&
            (static_cast<unsigned char>(c) <= 0x7e);
    }
};

constexpr target_chars_t target_chars{};

//------------------------------------------------

// WS-VCHAR = SP / HTAB / VCHAR
struct ws_vchars_t
{
    constexpr
    bool
    operator()(char ch) const noexcept
    {
        return (
            ch >= 0x20 && ch <= 0x7e) ||
            ch == 0x09;
    }
};

constexpr ws_vchars_t ws_vchars{};

//------------------------------------------------

// OWS         = *( SP / HTAB )
inline
void
skip_ows(
    char const*& it,
    char const* end) noexcept
{
    while(it != end)
    {
        if(! ws(*it))
            break;
        ++it;
    }
}

struct ows_rule_t
{
    using value_type = void;

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) noexcept
    {
        skip_ows(it, end);
        return system::error_code();
    }
};

constexpr ows_rule_t ows_rule{};

//------------------------------------------------

// CRLF            = CR LF
struct crlf_rule_t
{
    using value_type = void;

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};

constexpr crlf_rule_t crlf_rule{};

//------------------------------------------------

// HTTP-version    = "HTTP/" DIGIT "." DIGIT
struct version_rule_t
{
    using value_type = unsigned char;

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};

constexpr version_rule_t version_rule{};

//------------------------------------------------

// request-line    = method SP request-target SP HTTP-version CRLF
constexpr auto
request_line_rule =
    grammar::tuple_rule(
        token_rule,
        grammar::squelch(
            grammar::delim_rule(' ') ),
        grammar::token_rule(
            grammar::lut_chars(target_chars) ),
        grammar::squelch(
            grammar::delim_rule(' ') ),
        version_rule,
        crlf_rule);

//------------------------------------------------

// status-code     = 3DIGIT
struct status_code_rule_t
{
    struct value_type
    {
        int v;
        status st;
        core::string_view s;
    };

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};

constexpr status_code_rule_t status_code_rule{};

//------------------------------------------------

// status-code     = *( HTAB / SP / VCHAR / obs-text )
struct reason_phrase_rule_t
{
    using value_type = core::string_view;

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};

constexpr reason_phrase_rule_t reason_phrase_rule{};

//------------------------------------------------

// status-line     = HTTP-version SP status-code SP reason-phrase CRLF
constexpr auto
status_line_rule =
    grammar::tuple_rule(
        version_rule,
        grammar::squelch(
            grammar::delim_rule(' ') ),
        status_code_rule,
        grammar::squelch(
            grammar::delim_rule(' ') ),
        reason_phrase_rule,
        crlf_rule);

//------------------------------------------------

struct field_rule_t
{
    struct value_type
    {
        core::string_view name;
        core::string_view value;
        bool has_obs_fold = false;
    };

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};

constexpr field_rule_t field_rule{};

/** Replace obs-fold with spaces
*/
void
remove_obs_fold(
    char *start,
    char const* end) noexcept;

// header-field   = field-name ":" OWS field-value OWS
struct field_name_rule_t
{
    using value_type = core::string_view;

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};

constexpr field_name_rule_t field_name_rule{};

struct field_value_rule_t
{
    struct value_type
    {
        core::string_view value;
        // detected occurrence of `\r\n `, `\r\n\t`
        bool has_obs_fold = false;
        // detected `\r\nX`, attempt at field termination
        bool has_crlf = false;
    };

    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};

constexpr field_value_rule_t field_value_rule{};

} // detail
} // http
} // boost

#endif
