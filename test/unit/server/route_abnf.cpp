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

//------------------------------------------------

struct route_match_test
{
    using match_options = detail::match_options;
    using match_params = detail::match_params;

    // Helper to parse pattern
    static detail::route_pattern
    parse(std::string_view pat)
    {
        auto rv = detail::parse_route_pattern(pat);
        BOOST_TEST(rv.has_value());
        return std::move(rv.value());
    }

    // Helper to check param value
    static void
    check_param(
        match_params const& mp,
        std::string const& name,
        std::string const& value)
    {
        for(auto const& p : mp.params)
        {
            if(p.first == name)
            {
                BOOST_TEST_EQ(p.second, value);
                return;
            }
        }
        BOOST_TEST(false);  // param not found
    }

    // Default options for convenience
    static match_options
    opts(bool case_sensitive = false, bool strict = false, bool end = true)
    {
        return { case_sensitive, strict, end };
    }

    //--------------------------------------------
    // Text Matching
    //--------------------------------------------

    void
    testTextExact()
    {
        auto pat = parse("/users");

        // Exact match
        {
            auto rv = detail::match_route("/users", pat, opts());
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->matched_length, 6u);
            BOOST_TEST_EQ(rv->params.size(), 0u);
        }

        // No match - different text
        {
            auto rv = detail::match_route("/posts", pat, opts());
            BOOST_TEST(rv.has_error());
        }

        // No match - too short
        {
            auto rv = detail::match_route("/use", pat, opts());
            BOOST_TEST(rv.has_error());
        }

        // No match - too long (end=true)
        {
            auto rv = detail::match_route("/users/123", pat, opts());
            BOOST_TEST(rv.has_error());
        }
    }

    void
    testTextCaseSensitive()
    {
        auto pat = parse("/Users");

        // Case insensitive (default) - should match
        {
            auto rv = detail::match_route("/users", pat, opts(false));
            BOOST_TEST(rv.has_value());
        }

        // Case sensitive - should not match
        {
            auto rv = detail::match_route("/users", pat, opts(true));
            BOOST_TEST(rv.has_error());
        }

        // Case sensitive - exact match
        {
            auto rv = detail::match_route("/Users", pat, opts(true));
            BOOST_TEST(rv.has_value());
        }
    }

    void
    testTextRoot()
    {
        auto pat = parse("/");

        // Match root
        {
            auto rv = detail::match_route("/", pat, opts());
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->matched_length, 1u);
        }
    }

    void
    testTextEmpty()
    {
        // Empty pattern matches empty path
        auto pat = parse("");
        auto rv = detail::match_route("", pat, opts());
        BOOST_TEST(rv.has_value());
        BOOST_TEST_EQ(rv->matched_length, 0u);
    }

    //--------------------------------------------
    // Parameter Extraction
    //--------------------------------------------

    void
    testParamSingle()
    {
        auto pat = parse("/users/:id");

        // Match and extract
        {
            auto rv = detail::match_route("/users/123", pat, opts());
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->params.size(), 1u);
            check_param(*rv, "id", "123");
            BOOST_TEST_EQ(rv->matched_length, 10u);
        }

        // Match with longer value
        {
            auto rv = detail::match_route("/users/abc-def", pat, opts());
            BOOST_TEST(rv.has_value());
            check_param(*rv, "id", "abc-def");
        }
    }

    void
    testParamMultiple()
    {
        auto pat = parse("/users/:userId/posts/:postId");

        auto rv = detail::match_route("/users/42/posts/99", pat, opts());
        BOOST_TEST(rv.has_value());
        BOOST_TEST_EQ(rv->params.size(), 2u);
        check_param(*rv, "userId", "42");
        check_param(*rv, "postId", "99");
    }

    void
    testParamAdjacent()
    {
        // Adjacent params with slash separator
        // Note: params match until '/' only, not arbitrary text
        auto pat = parse("/:foo/:bar");

        auto rv = detail::match_route("/hello/world", pat, opts());
        BOOST_TEST(rv.has_value());
        BOOST_TEST_EQ(rv->params.size(), 2u);
        check_param(*rv, "foo", "hello");
        check_param(*rv, "bar", "world");
    }

    void
    testParamAtStart()
    {
        auto pat = parse(":foo/bar");

        auto rv = detail::match_route("hello/bar", pat, opts());
        BOOST_TEST(rv.has_value());
        check_param(*rv, "foo", "hello");
    }

    void
    testParamAtEnd()
    {
        auto pat = parse("/foo/:bar");

        auto rv = detail::match_route("/foo/baz", pat, opts());
        BOOST_TEST(rv.has_value());
        check_param(*rv, "bar", "baz");
    }

    void
    testParamEmpty()
    {
        // Param must capture at least one char
        auto pat = parse("/users/:id/posts");

        auto rv = detail::match_route("/users//posts", pat, opts());
        BOOST_TEST(rv.has_error());
    }

    //--------------------------------------------
    // Wildcard Extraction
    //--------------------------------------------

    void
    testWildcardSimple()
    {
        auto pat = parse("/files/*path");

        auto rv = detail::match_route("/files/a/b/c.txt", pat, opts());
        BOOST_TEST(rv.has_value());
        check_param(*rv, "path", "a/b/c.txt");
    }

    void
    testWildcardAtRoot()
    {
        auto pat = parse("/*path");

        auto rv = detail::match_route("/anything/here", pat, opts());
        BOOST_TEST(rv.has_value());
        check_param(*rv, "path", "anything/here");
    }

    void
    testWildcardEmpty()
    {
        // Wildcard must capture at least one char
        auto pat = parse("/files/*path");

        auto rv = detail::match_route("/files/", pat, opts());
        BOOST_TEST(rv.has_error());
    }

    //--------------------------------------------
    // Option Combinations (all 8)
    //--------------------------------------------

    void
    testOptionsCombinations()
    {
        auto pat = parse("/Api");

        // {case_sensitive: false, strict: false, end: true}
        {
            auto rv = detail::match_route("/api", pat, opts(false, false, true));
            BOOST_TEST(rv.has_value());
        }
        {
            auto rv = detail::match_route("/api/", pat, opts(false, false, true));
            BOOST_TEST(rv.has_value());  // trailing slash allowed
        }

        // {case_sensitive: false, strict: false, end: false}
        {
            auto rv = detail::match_route("/api/extra", pat, opts(false, false, false));
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->matched_length, 4u);
        }

        // {case_sensitive: false, strict: true, end: true}
        {
            auto rv = detail::match_route("/api", pat, opts(false, true, true));
            BOOST_TEST(rv.has_value());
        }
        {
            auto rv = detail::match_route("/api/", pat, opts(false, true, true));
            BOOST_TEST(rv.has_error());  // strict - trailing slash not allowed
        }

        // {case_sensitive: false, strict: true, end: false}
        {
            auto rv = detail::match_route("/api/extra", pat, opts(false, true, false));
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->matched_length, 4u);
        }

        // {case_sensitive: true, strict: false, end: true}
        {
            auto rv = detail::match_route("/Api", pat, opts(true, false, true));
            BOOST_TEST(rv.has_value());
        }
        {
            auto rv = detail::match_route("/api", pat, opts(true, false, true));
            BOOST_TEST(rv.has_error());  // case mismatch
        }

        // {case_sensitive: true, strict: false, end: false}
        {
            auto rv = detail::match_route("/Api/extra", pat, opts(true, false, false));
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->matched_length, 4u);
        }

        // {case_sensitive: true, strict: true, end: true}
        {
            auto rv = detail::match_route("/Api", pat, opts(true, true, true));
            BOOST_TEST(rv.has_value());
        }
        {
            auto rv = detail::match_route("/Api/", pat, opts(true, true, true));
            BOOST_TEST(rv.has_error());
        }

        // {case_sensitive: true, strict: true, end: false}
        {
            auto rv = detail::match_route("/Api/extra", pat, opts(true, true, false));
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->matched_length, 4u);
        }
    }

    //--------------------------------------------
    // Strict Mode
    //--------------------------------------------

    void
    testStrictTrailingSlash()
    {
        auto pat = parse("/api/users");

        // Non-strict: /api/users matches /api/users/
        {
            auto rv = detail::match_route("/api/users/", pat, opts(false, false, true));
            BOOST_TEST(rv.has_value());
        }

        // Strict: /api/users does NOT match /api/users/
        {
            auto rv = detail::match_route("/api/users/", pat, opts(false, true, true));
            BOOST_TEST(rv.has_error());
        }
    }

    void
    testStrictWithParam()
    {
        auto pat = parse("/users/:id");

        // Non-strict
        {
            auto rv = detail::match_route("/users/123/", pat, opts(false, false, true));
            BOOST_TEST(rv.has_value());
            check_param(*rv, "id", "123");
        }

        // Strict - trailing slash after param not allowed
        {
            auto rv = detail::match_route("/users/123/", pat, opts(false, true, true));
            BOOST_TEST(rv.has_error());
        }
    }

    //--------------------------------------------
    // End Mode (prefix vs full match)
    //--------------------------------------------

    void
    testEndModeFull()
    {
        auto pat = parse("/api");

        // end=true requires full match
        {
            auto rv = detail::match_route("/api", pat, opts(false, false, true));
            BOOST_TEST(rv.has_value());
        }
        {
            auto rv = detail::match_route("/api/users", pat, opts(false, false, true));
            BOOST_TEST(rv.has_error());
        }
    }

    void
    testEndModePrefix()
    {
        auto pat = parse("/api");

        // end=false allows prefix match
        {
            auto rv = detail::match_route("/api/users", pat, opts(false, false, false));
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->matched_length, 4u);
        }
    }

    void
    testEndModeWithParams()
    {
        auto pat = parse("/users/:id");

        // Prefix match with param
        {
            auto rv = detail::match_route("/users/123/extra", pat, opts(false, false, false));
            BOOST_TEST(rv.has_value());
            check_param(*rv, "id", "123");
            BOOST_TEST_EQ(rv->matched_length, 10u);
        }
    }

    //--------------------------------------------
    // Groups (Optional Sections)
    //--------------------------------------------

    void
    testGroupMatches()
    {
        auto pat = parse("/api{/v:version}");

        // With group
        {
            auto rv = detail::match_route("/api/v2", pat, opts());
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->params.size(), 1u);
            check_param(*rv, "version", "2");
        }

        // Without group
        {
            auto rv = detail::match_route("/api", pat, opts());
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->params.size(), 0u);
        }
    }

    void
    testGroupTextOnly()
    {
        auto pat = parse("/file{.json}");

        // With extension
        {
            auto rv = detail::match_route("/file.json", pat, opts());
            BOOST_TEST(rv.has_value());
        }

        // Without extension
        {
            auto rv = detail::match_route("/file", pat, opts());
            BOOST_TEST(rv.has_value());
        }
    }

    void
    testGroupNested()
    {
        auto pat = parse("/a{/b{/c}}");

        // All levels
        {
            auto rv = detail::match_route("/a/b/c", pat, opts());
            BOOST_TEST(rv.has_value());
        }

        // Two levels
        {
            auto rv = detail::match_route("/a/b", pat, opts());
            BOOST_TEST(rv.has_value());
        }

        // One level
        {
            auto rv = detail::match_route("/a", pat, opts());
            BOOST_TEST(rv.has_value());
        }
    }

    void
    testGroupMultiple()
    {
        auto pat = parse("{/a}{/b}");

        // Both groups
        {
            auto rv = detail::match_route("/a/b", pat, opts());
            BOOST_TEST(rv.has_value());
        }

        // First only
        {
            auto rv = detail::match_route("/a", pat, opts());
            BOOST_TEST(rv.has_value());
        }

        // Second only
        {
            auto rv = detail::match_route("/b", pat, opts());
            BOOST_TEST(rv.has_value());
        }

        // Neither
        {
            auto rv = detail::match_route("", pat, opts());
            BOOST_TEST(rv.has_value());
        }
    }

    void
    testGroupEmpty()
    {
        auto pat = parse("/path{}");

        auto rv = detail::match_route("/path", pat, opts());
        BOOST_TEST(rv.has_value());
    }

    void
    testGroupAtEnd()
    {
        auto pat = parse("/required{/optional}");

        // With optional
        {
            auto rv = detail::match_route("/required/optional", pat, opts());
            BOOST_TEST(rv.has_value());
        }

        // Without optional
        {
            auto rv = detail::match_route("/required", pat, opts());
            BOOST_TEST(rv.has_value());
        }
    }

    //--------------------------------------------
    // Non-Matching Cases
    //--------------------------------------------

    void
    testNonMatching()
    {
        auto pat = parse("/users/:id");

        // Path too short
        {
            auto rv = detail::match_route("/users", pat, opts());
            BOOST_TEST(rv.has_error());
        }

        // Wrong literal
        {
            auto rv = detail::match_route("/posts/123", pat, opts());
            BOOST_TEST(rv.has_error());
        }
    }

    //--------------------------------------------
    // Edge Cases
    //--------------------------------------------

    void
    testManyParams()
    {
        auto pat = parse("/:a/:b/:c/:d/:e/:f/:g/:h/:i/:j");

        auto rv = detail::match_route("/1/2/3/4/5/6/7/8/9/10", pat, opts());
        BOOST_TEST(rv.has_value());
        BOOST_TEST_EQ(rv->params.size(), 10u);
        check_param(*rv, "a", "1");
        check_param(*rv, "j", "10");
    }

    void
    testConsecutiveSlashes()
    {
        auto pat = parse("/a//b");

        auto rv = detail::match_route("/a//b", pat, opts());
        BOOST_TEST(rv.has_value());
    }

    //--------------------------------------------
    // Integration / Real-world patterns
    //--------------------------------------------

    void
    testExpressStyle()
    {
        auto pat = parse("/api/v1/users/:userId/posts/:postId");

        auto rv = detail::match_route("/api/v1/users/42/posts/99", pat, opts());
        BOOST_TEST(rv.has_value());
        check_param(*rv, "userId", "42");
        check_param(*rv, "postId", "99");
    }

    void
    testGitHubStyle()
    {
        auto pat = parse("/:owner/:repo/blob/:branch/*path");

        auto rv = detail::match_route("/john/myrepo/blob/main/src/index.js", pat, opts());
        BOOST_TEST(rv.has_value());
        check_param(*rv, "owner", "john");
        check_param(*rv, "repo", "myrepo");
        check_param(*rv, "branch", "main");
        check_param(*rv, "path", "src/index.js");
    }

    void
    testFileExtension()
    {
        auto pat = parse("/file{.:ext}");

        // With extension
        {
            auto rv = detail::match_route("/file.txt", pat, opts());
            BOOST_TEST(rv.has_value());
            check_param(*rv, "ext", "txt");
        }

        // Without extension
        {
            auto rv = detail::match_route("/file", pat, opts());
            BOOST_TEST(rv.has_value());
            BOOST_TEST_EQ(rv->params.size(), 0u);
        }
    }

    void
    run()
    {
        // Text matching
        testTextExact();
        testTextCaseSensitive();
        testTextRoot();
        testTextEmpty();

        // Parameter extraction
        testParamSingle();
        testParamMultiple();
        testParamAdjacent();
        testParamAtStart();
        testParamAtEnd();
        testParamEmpty();

        // Wildcard extraction
        testWildcardSimple();
        testWildcardAtRoot();
        testWildcardEmpty();

        // Option combinations
        testOptionsCombinations();

        // Strict mode
        testStrictTrailingSlash();
        testStrictWithParam();

        // End mode
        testEndModeFull();
        testEndModePrefix();
        testEndModeWithParams();

        // Groups
        testGroupMatches();
        testGroupTextOnly();
        testGroupNested();
        testGroupMultiple();
        testGroupEmpty();
        testGroupAtEnd();

        // Non-matching
        testNonMatching();

        // Edge cases
        testManyParams();
        testConsecutiveSlashes();

        // Integration
        testExpressStyle();
        testGitHubStyle();
        testFileExtension();
    }
};

TEST_SUITE(
    route_match_test,
    "boost.http.server.route_match");

} // http
} // boost
