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

//------------------------------------------------
// Case-insensitive comparison
//------------------------------------------------

bool
ci_equal(char a, char b) noexcept
{
    if(a >= 'A' && a <= 'Z')
        a = static_cast<char>(a + 32);
    if(b >= 'A' && b <= 'Z')
        b = static_cast<char>(b + 32);
    return a == b;
}

bool
ci_starts_with(
    core::string_view str,
    core::string_view prefix) noexcept
{
    if(prefix.size() > str.size())
        return false;
    for(std::size_t i = 0; i < prefix.size(); ++i)
    {
        if(!ci_equal(str[i], prefix[i]))
            return false;
    }
    return true;
}

//------------------------------------------------
// Route matcher
//------------------------------------------------

class route_matcher
{
    core::string_view path_;
    match_options const& opts_;
    std::vector<std::pair<std::string, std::string>> params_;
    std::size_t pos_ = 0;

public:
    route_matcher(
        core::string_view path,
        match_options const& opts)
        : path_(path)
        , opts_(opts)
    {
    }

    bool at_end() const noexcept
    {
        return pos_ >= path_.size();
    }

    std::size_t pos() const noexcept
    {
        return pos_;
    }

    std::vector<std::pair<std::string, std::string>> const&
    params() const noexcept
    {
        return params_;
    }

    // Match text token
    bool match_text(core::string_view text)
    {
        auto remaining = path_.substr(pos_);
        if(opts_.case_sensitive)
        {
            if(!remaining.starts_with(text))
                return false;
        }
        else
        {
            if(!ci_starts_with(remaining, text))
                return false;
        }
        pos_ += text.size();
        return true;
    }

    // Match param token - capture until stop_char, '/' or end
    bool match_param(std::string const& name, char stop_char = '\0')
    {
        if(at_end())
            return false;

        auto start = pos_;
        while(pos_ < path_.size() && path_[pos_] != '/')
        {
            // Stop at delimiter if specified
            if(stop_char != '\0' && path_[pos_] == stop_char)
                break;
            ++pos_;
        }

        // Param must capture at least one character
        if(pos_ == start)
            return false;

        params_.emplace_back(
            name,
            std::string(path_.substr(start, pos_ - start)));
        return true;
    }

    // Match wildcard token - capture everything to end
    bool match_wildcard(std::string const& name)
    {
        if(at_end())
            return false;

        auto start = pos_;
        pos_ = path_.size();

        // Wildcard must capture at least one character
        if(pos_ == start)
            return false;

        params_.emplace_back(
            name,
            std::string(path_.substr(start)));
        return true;
    }

    // Get the first character of the next meaningful token
    // Returns '\0' if none exists or next token is not text
    static char
    get_stop_char(
        std::vector<route_token> const& tokens,
        std::size_t next_idx)
    {
        if(next_idx >= tokens.size())
            return '\0';

        auto const& next = tokens[next_idx];
        if(next.type == route_token_type::text && !next.value.empty())
            return next.value[0];

        return '\0';
    }

    // Match a sequence of tokens
    bool match_tokens(std::vector<route_token> const& tokens)
    {
        for(std::size_t i = 0; i < tokens.size(); ++i)
        {
            if(!match_token(tokens[i], get_stop_char(tokens, i + 1)))
                return false;
        }
        return true;
    }

    // Match a single token
    bool match_token(route_token const& token, char stop_char = '\0')
    {
        switch(token.type)
        {
        case route_token_type::text:
            return match_text(token.value);

        case route_token_type::param:
            return match_param(token.value, stop_char);

        case route_token_type::wildcard:
            return match_wildcard(token.value);

        case route_token_type::group:
            return match_group(token.children);

        default:
            return false;
        }
    }

    // Match group - try with contents, then without
    bool match_group(std::vector<route_token> const& children)
    {
        // Save state before trying group
        auto saved_pos = pos_;
        auto saved_params_size = params_.size();

        // Try matching with group contents
        if(match_tokens(children))
            return true;

        // Restore state and try without group
        pos_ = saved_pos;
        params_.resize(saved_params_size);
        return true;  // Group is optional, always succeeds if skipped
    }

    // Check if match is complete based on options
    bool is_complete() const
    {
        if(!opts_.end)
            return true;  // Prefix match always succeeds

        if(opts_.strict)
            return at_end();

        // Non-strict: allow trailing slash
        if(at_end())
            return true;
        if(pos_ == path_.size() - 1 && path_[pos_] == '/')
            return true;

        return false;
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

//------------------------------------------------

system::result<match_params>
match_route(
    core::string_view path,
    route_pattern const& pattern,
    match_options const& opts)
{
    route_matcher m(path, opts);

    if(!m.match_tokens(pattern.tokens))
        return grammar::error::mismatch;

    if(!m.is_complete())
        return grammar::error::mismatch;

    match_params result;
    result.params = m.params();
    result.matched_length = m.pos();
    return result;
}

} // detail
} // http
} // boost
