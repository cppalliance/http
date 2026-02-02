//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained
#include "src/server/route_abnf.hpp"

#include "test_suite.hpp"

namespace boost {
namespace http {

struct route_abnf_test
{
    using token = detail::route_token;
    using token_type = detail::route_token_type;

    // Helper to check token type and value
    static void
    check_token(
        token const& t,
        token_type type,
        std::string const& value)
    {
        BOOST_TEST_EQ(static_cast<int>(t.type), static_cast<int>(type));
        BOOST_TEST_EQ(t.value, value);
    }

    void
    testText()
    {
        // Simple text
        {
            auto rv = detail::parse_route_pattern("/users");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::text, "/users");
        }

        // Text with multiple segments
        {
            auto rv = detail::parse_route_pattern("/api/v1/users");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::text, "/api/v1/users");
        }
    }

    void
    testParam()
    {
        // Simple param
        {
            auto rv = detail::parse_route_pattern(":id");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::param, "id");
        }

        // Param with text prefix
        {
            auto rv = detail::parse_route_pattern("/users/:id");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 2u);
            check_token(rv->tokens[0], token_type::text, "/users/");
            check_token(rv->tokens[1], token_type::param, "id");
        }

        // Multiple params
        {
            auto rv = detail::parse_route_pattern("/users/:userId/posts/:postId");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 4u);
            check_token(rv->tokens[0], token_type::text, "/users/");
            check_token(rv->tokens[1], token_type::param, "userId");
            check_token(rv->tokens[2], token_type::text, "/posts/");
            check_token(rv->tokens[3], token_type::param, "postId");
        }

        // Param with underscore
        {
            auto rv = detail::parse_route_pattern(":user_id");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::param, "user_id");
        }

        // Param with dollar sign
        {
            auto rv = detail::parse_route_pattern(":$var");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::param, "$var");
        }
    }

    void
    testWildcard()
    {
        // Simple wildcard
        {
            auto rv = detail::parse_route_pattern("*path");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::wildcard, "path");
        }

        // Wildcard with prefix
        {
            auto rv = detail::parse_route_pattern("/files/*filepath");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 2u);
            check_token(rv->tokens[0], token_type::text, "/files/");
            check_token(rv->tokens[1], token_type::wildcard, "filepath");
        }
    }

    void
    testGroup()
    {
        // Simple group
        {
            auto rv = detail::parse_route_pattern("{/optional}");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            BOOST_TEST_EQ(
                static_cast<int>(rv->tokens[0].type),
                static_cast<int>(token_type::group));
            BOOST_TEST_EQ(rv->tokens[0].children.size(), 1u);
            check_token(rv->tokens[0].children[0], token_type::text, "/optional");
        }

        // Group with param
        {
            auto rv = detail::parse_route_pattern("/api{/v:version}");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 2u);
            check_token(rv->tokens[0], token_type::text, "/api");
            BOOST_TEST_EQ(
                static_cast<int>(rv->tokens[1].type),
                static_cast<int>(token_type::group));
            BOOST_TEST_EQ(rv->tokens[1].children.size(), 2u);
            check_token(rv->tokens[1].children[0], token_type::text, "/v");
            check_token(rv->tokens[1].children[1], token_type::param, "version");
        }

        // Empty group
        {
            auto rv = detail::parse_route_pattern("/path{}");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 2u);
            check_token(rv->tokens[0], token_type::text, "/path");
            BOOST_TEST_EQ(rv->tokens[1].children.size(), 0u);
        }
    }

    void
    testEscape()
    {
        // Escaped colon
        {
            auto rv = detail::parse_route_pattern("/path\\:literal");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::text, "/path:literal");
        }

        // Escaped asterisk
        {
            auto rv = detail::parse_route_pattern("/path\\*star");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::text, "/path*star");
        }

        // Escaped brace
        {
            auto rv = detail::parse_route_pattern("/path\\{brace\\}");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::text, "/path{brace}");
        }

        // Escaped backslash
        {
            auto rv = detail::parse_route_pattern("/path\\\\slash");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::text, "/path\\slash");
        }
    }

    void
    testQuotedName()
    {
        // Quoted param name
        {
            auto rv = detail::parse_route_pattern(":\"with spaces\"");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::param, "with spaces");
        }

        // Quoted wildcard name
        {
            auto rv = detail::parse_route_pattern("*\"file-path\"");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::wildcard, "file-path");
        }

        // Quoted name with escape
        {
            auto rv = detail::parse_route_pattern(":\"say \\\"hello\\\"\"");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 1u);
            check_token(rv->tokens[0], token_type::param, "say \"hello\"");
        }
    }

    void
    testErrors()
    {
        // Missing param name
        {
            auto rv = detail::parse_route_pattern("/users/:");
            BOOST_TEST(rv.has_error());
        }

        // Missing wildcard name
        {
            auto rv = detail::parse_route_pattern("/files/*");
            BOOST_TEST(rv.has_error());
        }

        // Unclosed group
        {
            auto rv = detail::parse_route_pattern("/path{unclosed");
            BOOST_TEST(rv.has_error());
        }

        // Unexpected close brace
        {
            auto rv = detail::parse_route_pattern("/path}extra");
            BOOST_TEST(rv.has_error());
        }

        // Unterminated quote
        {
            auto rv = detail::parse_route_pattern(":\"unterminated");
            BOOST_TEST(rv.has_error());
        }

        // Empty quoted name
        {
            auto rv = detail::parse_route_pattern(":\"\"");
            BOOST_TEST(rv.has_error());
        }

        // Reserved character (
        {
            auto rv = detail::parse_route_pattern("/path(reserved)");
            BOOST_TEST(rv.has_error());
        }

        // Reserved character [
        {
            auto rv = detail::parse_route_pattern("/path[reserved]");
            BOOST_TEST(rv.has_error());
        }

        // Reserved character +
        {
            auto rv = detail::parse_route_pattern("/path+");
            BOOST_TEST(rv.has_error());
        }

        // Reserved character ?
        {
            auto rv = detail::parse_route_pattern("/path?");
            BOOST_TEST(rv.has_error());
        }

        // Reserved character !
        {
            auto rv = detail::parse_route_pattern("/path!");
            BOOST_TEST(rv.has_error());
        }

        // Trailing backslash
        {
            auto rv = detail::parse_route_pattern("/path\\");
            BOOST_TEST(rv.has_error());
        }
    }

    void
    testComplex()
    {
        // Express.js style route
        {
            auto rv = detail::parse_route_pattern(
                "/api/v1/users/:userId/posts/:postId");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 4u);
        }

        // Multiple consecutive params with separator
        {
            auto rv = detail::parse_route_pattern("/:foo-:bar");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 4u);
            check_token(rv->tokens[0], token_type::text, "/");
            check_token(rv->tokens[1], token_type::param, "foo");
            check_token(rv->tokens[2], token_type::text, "-");
            check_token(rv->tokens[3], token_type::param, "bar");
        }

        // Nested groups not directly supported but works as single group
        {
            auto rv = detail::parse_route_pattern("/path{/opt1{/opt2}}");
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->tokens.size(), 2u);
            // The outer group contains text + nested group
            BOOST_TEST_EQ(rv->tokens[1].children.size(), 2u);
        }
    }

    void
    testOriginalPreserved()
    {
        auto rv = detail::parse_route_pattern("/users/:id");
        BOOST_TEST(rv.has_value());
        BOOST_TEST_EQ(rv->original, "/users/:id");
    }

    void
    run()
    {
        testText();
        testParam();
        testWildcard();
        testGroup();
        testEscape();
        testQuotedName();
        testErrors();
        testComplex();
        testOriginalPreserved();
    }
};

TEST_SUITE(
    route_abnf_test,
    "boost.http.server.route_abnf");

} // http
} // boost
