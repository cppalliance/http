//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/server/route_abnf.hpp"
#include <boost/url/grammar/error.hpp>

namespace boost {
namespace http {
namespace detail {

namespace {

//------------------------------------------------
// Character classification
//------------------------------------------------

// Special characters that have meaning in patterns
constexpr bool
is_special(char c) noexcept
{
    switch(c)
    {
    case '{':
    case '}':
    case '(':
    case ')':
    case '[':
    case ']':
    case '+':
    case '?':
    case '!':
    case ':':
    case '*':
    case '\\':
        return true;
    default:
        return false;
    }
}

// Reserved characters (parsed but invalid)
constexpr bool
is_reserved(char c) noexcept
{
    switch(c)
    {
    case '(':
    case ')':
    case '[':
    case ']':
    case '+':
    case '?':
    case '!':
        return true;
    default:
        return false;
    }
}

// Valid identifier start (ASCII subset of ID_Start)
constexpr bool
is_id_start(char c) noexcept
{
    return
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_' || c == '$';
}

// Valid identifier continuation (ASCII subset of ID_Continue)
constexpr bool
is_id_continue(char c) noexcept
{
    return
        is_id_start(c) ||
        (c >= '0' && c <= '9');
}

//------------------------------------------------
// Parser state
//------------------------------------------------

class parser
{
    char const* it_;
    char const* end_;
    core::string_view original_;

public:
    parser(core::string_view s)
        : it_(s.data())
        , end_(s.data() + s.size())
        , original_(s)
    {
    }

    bool
    at_end() const noexcept
    {
        return it_ == end_;
    }

    char
    peek() const noexcept
    {
        return *it_;
    }

    void
    advance() noexcept
    {
        ++it_;
    }

    char
    get() noexcept
    {
        return *it_++;
    }

    std::size_t
    pos() const noexcept
    {
        return static_cast<std::size_t>(
            it_ - original_.data());
    }

    //--------------------------------------------
    // Name parsing
    //--------------------------------------------

    // Parse identifier: id-start *id-continue
    system::result<std::string>
    parse_identifier()
    {
        if(at_end() || !is_id_start(peek()))
            return grammar::error::mismatch;

        std::string result;
        result += get();

        while(!at_end() && is_id_continue(peek()))
            result += get();

        return result;
    }

    // Parse quoted name: DQUOTE *quoted-char DQUOTE
    system::result<std::string>
    parse_quoted_name()
    {
        if(at_end() || peek() != '"')
            return grammar::error::mismatch;

        advance(); // skip opening quote
        std::string result;

        while(!at_end())
        {
            char c = peek();

            if(c == '"')
            {
                advance(); // skip closing quote
                if(result.empty())
                    return grammar::error::syntax;
                return result;
            }

            if(c == '\\')
            {
                advance(); // skip backslash
                if(at_end())
                    return grammar::error::syntax;
                result += get();
            }
            else
            {
                result += get();
            }
        }

        // Unterminated quote
        return grammar::error::syntax;
    }

    // Parse name: identifier / quoted-name
    system::result<std::string>
    parse_name()
    {
        if(at_end())
            return grammar::error::syntax;

        if(peek() == '"')
            return parse_quoted_name();

        return parse_identifier();
    }

    //--------------------------------------------
    // Token parsing
    //--------------------------------------------

    // Parse text: 1*(char / escaped-char)
    system::result<route_token>
    parse_text()
    {
        std::string result;

        while(!at_end())
        {
            char c = peek();

            // Stop at special characters
            if(is_special(c))
            {
                if(c == '\\')
                {
                    // Escaped character
                    advance();
                    if(at_end())
                        return grammar::error::syntax;
                    result += get();
                    continue;
                }
                break;
            }

            result += get();
        }

        if(result.empty())
            return grammar::error::mismatch;

        return route_token(route_token_type::text, std::move(result));
    }

    // Parse param: ":" name
    system::result<route_token>
    parse_param()
    {
        if(at_end() || peek() != ':')
            return grammar::error::mismatch;

        advance(); // skip ':'

        auto rv = parse_name();
        if(rv.has_error())
            return rv.error();

        return route_token(
            route_token_type::param, std::move(rv.value()));
    }

    // Parse wildcard: "*" name
    system::result<route_token>
    parse_wildcard()
    {
        if(at_end() || peek() != '*')
            return grammar::error::mismatch;

        advance(); // skip '*'

        auto rv = parse_name();
        if(rv.has_error())
            return rv.error();

        return route_token(
            route_token_type::wildcard, std::move(rv.value()));
    }

    // Parse group: "{" *token "}"
    system::result<route_token>
    parse_group()
    {
        if(at_end() || peek() != '{')
            return grammar::error::mismatch;

        advance(); // skip '{'

        route_token group;
        group.type = route_token_type::group;

        // Parse tokens until '}'
        while(!at_end() && peek() != '}')
        {
            auto rv = parse_token();
            if(rv.has_error())
                return rv.error();
            group.children.push_back(std::move(rv.value()));
        }

        if(at_end())
            return grammar::error::syntax; // unclosed group

        advance(); // skip '}'

        return group;
    }

    // Parse single token
    system::result<route_token>
    parse_token()
    {
        if(at_end())
            return grammar::error::syntax;

        char c = peek();

        // Check for reserved characters
        if(is_reserved(c))
            return grammar::error::syntax;

        // Try each token type
        if(c == ':')
            return parse_param();

        if(c == '*')
            return parse_wildcard();

        if(c == '{')
            return parse_group();

        if(c == '}')
            return grammar::error::syntax; // unexpected '}'

        // Must be text
        return parse_text();
    }

    // Parse entire pattern
    system::result<std::vector<route_token>>
    parse_tokens()
    {
        std::vector<route_token> tokens;

        while(!at_end())
        {
            auto rv = parse_token();
            if(rv.has_error())
                return rv.error();
            tokens.push_back(std::move(rv.value()));
        }

        return tokens;
    }
};

} // anonymous namespace

//------------------------------------------------

system::result<route_pattern>
parse_route_pattern(core::string_view pattern)
{
    parser p(pattern);
    auto rv = p.parse_tokens();
    if(rv.has_error())
        return rv.error();

    route_pattern result;
    result.tokens = std::move(rv.value());
    result.original = std::string(pattern);
    return result;
}

} // detail
} // http
} // boost
