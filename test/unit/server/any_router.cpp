//
// Copyright (c) 2026 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/any_router.hpp>

// Full functional tests are in beast2/test/unit/server/router.cpp

#include <boost/http/server/basic_router.hpp>
#include <boost/http/server/router.hpp>

#include <boost/capy/test/run_blocking.hpp>
#include "test_suite.hpp"

namespace boost {
namespace http {

struct any_router_test
{
    using params = route_params;
    using test_router = basic_router<params>;

    void testCopyConstruction()
    {
        auto counter = std::make_shared<int>(0);
        test_router r;
        r.all("/", [counter](params&) -> route_task
        {
            ++(*counter);
            co_return route_result{};
        });

        any_router ar1(r);
        any_router ar2(ar1);

        params req;
        capy::test::run_blocking()(ar1.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 1);

        capy::test::run_blocking()(ar2.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 2);
    }

    void testCopyAssignment()
    {
        auto counter = std::make_shared<int>(0);
        test_router r;
        r.all("/", [counter](params&) -> route_task
        {
            ++(*counter);
            co_return route_result{};
        });

        any_router ar1(r);

        test_router r2;
        r2.all("/", [](params&) -> route_task
        {
            co_return route_result{};
        });
        any_router ar2(r2);

        ar2 = ar1;

        params req;
        capy::test::run_blocking()(ar1.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 1);

        capy::test::run_blocking()(ar2.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 2);
    }

    void testDefaultConstruction()
    {
        auto counter = std::make_shared<int>(0);
        test_router r;
        r.all("/", [counter](params&) -> route_task
        {
            ++(*counter);
            co_return route_result{};
        });

        any_router ar1;  // default construct
        any_router ar2(r);
        ar1 = ar2;  // assign to default-constructed

        params req;
        capy::test::run_blocking()(ar1.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST_EQ(*counter, 1);
    }

    void testOptionsHandler()
    {
        std::string captured_allow;
        test_router r;
        r.add(http::method::get, "/api/users", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.add(http::method::post, "/api/users", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.set_options_handler(
            [&captured_allow](params&, std::string_view allow) -> route_task
            {
                captured_allow = allow;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::options, urls::url_view("/api/users"), req));
        BOOST_TEST(captured_allow.find("GET") != std::string::npos);
        BOOST_TEST(captured_allow.find("POST") != std::string::npos);
    }

    void testExplicitOptionsPriority()
    {
        bool explicit_called = false;
        bool fallback_called = false;

        test_router r;
        r.add(http::method::get, "/test", [](params&) -> route_task
        {
            co_return route_done;
        });
        // Explicit OPTIONS handler
        r.add(http::method::options, "/test",
            [&explicit_called](params&) -> route_task
            {
                explicit_called = true;
                co_return route_done;
            });
        // Fallback handler
        r.set_options_handler(
            [&fallback_called](params&, std::string_view) -> route_task
            {
                fallback_called = true;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::options, urls::url_view("/test"), req));
        BOOST_TEST(explicit_called);
        BOOST_TEST(!fallback_called);
    }

    void testAllMethodsHandler()
    {
        std::string captured_allow;
        test_router r;
        // Use route().all() but have handler return route_next
        // so OPTIONS fallback can run
        r.route("/wildcard").all([](params&) -> route_task
        {
            co_return route_next;
        });
        r.set_options_handler(
            [&captured_allow](params&, std::string_view allow) -> route_task
            {
                captured_allow = allow;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::options, urls::url_view("/wildcard"), req));
        // .all() should produce a full Allow header
        BOOST_TEST(captured_allow.find("GET") != std::string::npos);
        BOOST_TEST(captured_allow.find("POST") != std::string::npos);
        BOOST_TEST(captured_allow.find("DELETE") != std::string::npos);
    }

    void testOptionsStarGlobal()
    {
        std::string captured_allow;
        test_router r;
        r.add(http::method::get, "/a", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.add(http::method::post, "/b", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.add(http::method::put, "/c", [](params&) -> route_task
        {
            co_return route_done;
        });
        r.set_options_handler(
            [&captured_allow](params&, std::string_view allow) -> route_task
            {
                captured_allow = allow;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::options, urls::url_view("*"), req));
        // Should contain all registered methods
        BOOST_TEST(captured_allow.find("GET") != std::string::npos);
        BOOST_TEST(captured_allow.find("POST") != std::string::npos);
        BOOST_TEST(captured_allow.find("PUT") != std::string::npos);
    }

    //--------------------------------------------
    // Route Pattern Integration Tests
    //--------------------------------------------

    // Helper to find param value by name
    static std::string
    get_param(params const& p, std::string const& name)
    {
        for(auto const& kv : p.params)
            if(kv.first == name)
                return kv.second;
        return "";
    }

    // No-op handler for pattern validation tests
    static route_task noop(params&) { co_return route_done; }

    void testParamCapture()
    {
        bool handler_called = false;
        std::string captured_id;

        test_router r;
        r.add(http::method::get, "/users/:id",
            [&](params& p) -> route_task
            {
                handler_called = true;
                captured_id = get_param(p, "id");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/users/123"), req));

        BOOST_TEST(handler_called);
        BOOST_TEST_EQ(captured_id, "123");
        BOOST_TEST_EQ(req.params.size(), 1u);
    }

    void testMultipleParams()
    {
        std::string captured_user;
        std::string captured_post;

        test_router r;
        r.add(http::method::get, "/users/:userId/posts/:postId",
            [&](params& p) -> route_task
            {
                captured_user = get_param(p, "userId");
                captured_post = get_param(p, "postId");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/users/42/posts/99"), req));

        BOOST_TEST_EQ(captured_user, "42");
        BOOST_TEST_EQ(captured_post, "99");
        BOOST_TEST_EQ(req.params.size(), 2u);
    }

    void testWildcardCapture()
    {
        std::string captured_path;

        test_router r;
        r.add(http::method::get, "/files/*filepath",
            [&](params& p) -> route_task
            {
                captured_path = get_param(p, "filepath");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/files/a/b/c.txt"), req));

        BOOST_TEST_EQ(captured_path, "a/b/c.txt");
    }

    void testOptionalGroup()
    {
        std::string captured_version;
        int call_count = 0;

        test_router r;
        r.add(http::method::get, "/api{/v:version}",
            [&](params& p) -> route_task
            {
                ++call_count;
                captured_version = get_param(p, "version");
                co_return route_done;
            });

        // With optional group
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api/v2"), req));
            BOOST_TEST_EQ(call_count, 1);
            BOOST_TEST_EQ(captured_version, "2");
        }

        // Without optional group
        {
            params req;
            captured_version.clear();
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api"), req));
            BOOST_TEST_EQ(call_count, 2);
            BOOST_TEST(captured_version.empty());
        }
    }

    void testParamWithDash()
    {
        // Param values can contain dashes
        std::string captured_id;

        test_router r;
        r.add(http::method::get, "/items/:id",
            [&](params& p) -> route_task
            {
                captured_id = get_param(p, "id");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/items/abc-def-123"), req));

        BOOST_TEST_EQ(captured_id, "abc-def-123");
    }

    void testNoMatch()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/users/:id",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Wrong path
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/posts/123"), req));
            BOOST_TEST(!handler_called);
        }
    }

    void testGitHubStyleRoute()
    {
        std::string owner, repo, branch, filepath;

        test_router r;
        r.add(http::method::get, "/:owner/:repo/blob/:branch/*path",
            [&](params& p) -> route_task
            {
                owner = get_param(p, "owner");
                repo = get_param(p, "repo");
                branch = get_param(p, "branch");
                filepath = get_param(p, "path");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get,
            urls::url_view("/john/myrepo/blob/main/src/index.js"),
            req));

        BOOST_TEST_EQ(owner, "john");
        BOOST_TEST_EQ(repo, "myrepo");
        BOOST_TEST_EQ(branch, "main");
        BOOST_TEST_EQ(filepath, "src/index.js");
        BOOST_TEST_EQ(req.params.size(), 4u);
    }

    void testFileExtensionGroup()
    {
        std::string captured_ext;
        int call_count = 0;

        test_router r;
        r.add(http::method::get, "/file{.:ext}",
            [&](params& p) -> route_task
            {
                ++call_count;
                captured_ext = get_param(p, "ext");
                co_return route_done;
            });

        // With extension
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/file.txt"), req));
            BOOST_TEST_EQ(call_count, 1);
            BOOST_TEST_EQ(captured_ext, "txt");
        }

        // Without extension
        {
            params req;
            captured_ext.clear();
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/file"), req));
            BOOST_TEST_EQ(call_count, 2);
            BOOST_TEST(captured_ext.empty());
        }
    }

    void testParamsClearedBetweenRequests()
    {
        test_router r;
        r.add(http::method::get, "/a/:id",
            [](params&) -> route_task { co_return route_done; });
        r.add(http::method::get, "/b",
            [](params&) -> route_task { co_return route_done; });

        params req;

        // First request captures param
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/a/123"), req));
        BOOST_TEST_EQ(req.params.size(), 1u);

        // Second request has no params - should be cleared
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/b"), req));
        BOOST_TEST_EQ(req.params.size(), 0u);
    }

    void testUrlEncodedParam()
    {
        std::string captured_name;

        test_router r;
        r.add(http::method::get, "/users/:name",
            [&](params& p) -> route_task
            {
                captured_name = get_param(p, "name");
                co_return route_done;
            });

        params req;
        // %20 = space, path is decoded before matching
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/users/john%20doe"), req));

        BOOST_TEST_EQ(captured_name, "john doe");
    }

    //--------------------------------------------
    // Path Adjustment Tests
    //--------------------------------------------

    void testPathAdjustmentSimple()
    {
        core::string_view captured_base;
        core::string_view captured_path;

        test_router r;
        r.use("/api",
            [&](params& p) -> route_task
            {
                captured_base = p.base_path;
                captured_path = p.path;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/api/v1/users"), req));

        BOOST_TEST_EQ(captured_base, "/api");
        BOOST_TEST_EQ(captured_path, "/v1/users");
    }

    void testPathAdjustmentNested2Levels()
    {
        core::string_view captured_base;
        core::string_view captured_path;

        test_router r;
        r.use("/api", [&]{
            test_router r2;
            r2.use("/v1",
                [&](params& p) -> route_task
                {
                    captured_base = p.base_path;
                    captured_path = p.path;
                    co_return route_done;
                });
            return r2;
        }());

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/api/v1/users"), req));

        BOOST_TEST_EQ(captured_base, "/api/v1");
        BOOST_TEST_EQ(captured_path, "/users");
    }

    void testPathAdjustmentNested3Levels()
    {
        core::string_view captured_base;
        core::string_view captured_path;

        test_router r;
        r.use("/api", [&]{
            test_router r2;
            r2.use("/v1", [&]{
                test_router r3;
                r3.use("/users",
                    [&](params& p) -> route_task
                    {
                        captured_base = p.base_path;
                        captured_path = p.path;
                        co_return route_done;
                    });
                return r3;
            }());
            return r2;
        }());

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/api/v1/users/123"), req));

        BOOST_TEST_EQ(captured_base, "/api/v1/users");
        BOOST_TEST_EQ(captured_path, "/123");
    }

    void testPathAdjustmentNested4Levels()
    {
        core::string_view captured_base;
        core::string_view captured_path;

        test_router r;
        r.use("/a", [&]{
            test_router r2;
            r2.use("/b", [&]{
                test_router r3;
                r3.use("/c", [&]{
                    test_router r4;
                    r4.use("/d",
                        [&](params& p) -> route_task
                        {
                            captured_base = p.base_path;
                            captured_path = p.path;
                            co_return route_done;
                        });
                    return r4;
                }());
                return r3;
            }());
            return r2;
        }());

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/a/b/c/d/e/f"), req));

        BOOST_TEST_EQ(captured_base, "/a/b/c/d");
        BOOST_TEST_EQ(captured_path, "/e/f");
    }

    void testPathAdjustmentWithRoute()
    {
        core::string_view captured_base;
        core::string_view captured_path;

        test_router r;
        r.use("/api", [&]{
            test_router r2;
            r2.use("/v1", [&]{
                test_router r3;
                r3.add(http::method::get, "/users/:id",
                    [&](params& p) -> route_task
                    {
                        captured_base = p.base_path;
                        captured_path = p.path;
                        co_return route_done;
                    });
                return r3;
            }());
            return r2;
        }());

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/api/v1/users/42"), req));

        BOOST_TEST_EQ(captured_base, "/api/v1/users/42");
        BOOST_TEST_EQ(captured_path, "/");
        BOOST_TEST_EQ(get_param(req, "id"), "42");
    }

    void testPathAdjustmentLongUrl()
    {
        // Tests path adjustment with URL longer than SSO threshold
        // to verify no dangling string_view after push_back reallocation
        core::string_view captured_base;
        core::string_view captured_path;

        test_router r;
        r.use("/very/long/path/prefix", [&]{
            test_router r2;
            r2.use("/that/exceeds", [&]{
                test_router r3;
                r3.use("/small/string/optimization",
                    [&](params& p) -> route_task
                    {
                        captured_base = p.base_path;
                        captured_path = p.path;
                        co_return route_done;
                    });
                return r3;
            }());
            return r2;
        }());

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get,
            urls::url_view("/very/long/path/prefix/that/exceeds/small/string/optimization/tail"),
            req));

        BOOST_TEST_EQ(captured_base, "/very/long/path/prefix/that/exceeds/small/string/optimization");
        BOOST_TEST_EQ(captured_path, "/tail");
    }

    //--------------------------------------------
    // Invalid Pattern Tests
    //--------------------------------------------

    void testInvalidPatternMissingParamName()
    {
        test_router r;
        BOOST_TEST_THROWS(
            r.add(http::method::get, "/users/:", noop),
            std::exception);
    }

    void testInvalidPatternMissingWildcardName()
    {
        test_router r;
        BOOST_TEST_THROWS(
            r.add(http::method::get, "/files/*", noop),
            std::exception);
    }

    void testInvalidPatternUnclosedGroup()
    {
        test_router r;
        BOOST_TEST_THROWS(
            r.add(http::method::get, "/path{unclosed", noop),
            std::exception);
    }

    void testInvalidPatternUnexpectedCloseBrace()
    {
        test_router r;
        BOOST_TEST_THROWS(
            r.add(http::method::get, "/path}extra", noop),
            std::exception);
    }

    void testInvalidPatternUnterminatedQuote()
    {
        test_router r;
        BOOST_TEST_THROWS(
            r.add(http::method::get, ":\"unterminated", noop),
            std::exception);
    }

    void testInvalidPatternEmptyQuotedName()
    {
        test_router r;
        BOOST_TEST_THROWS(
            r.add(http::method::get, ":\"\"", noop),
            std::exception);
    }

    void testInvalidPatternReservedChars()
    {
        test_router r;
        BOOST_TEST_THROWS(r.add(http::method::get, "/path(x)", noop), std::exception);
        BOOST_TEST_THROWS(r.add(http::method::get, "/path[x]", noop), std::exception);
        BOOST_TEST_THROWS(r.add(http::method::get, "/path+", noop), std::exception);
        BOOST_TEST_THROWS(r.add(http::method::get, "/path?", noop), std::exception);
        BOOST_TEST_THROWS(r.add(http::method::get, "/path!", noop), std::exception);
    }

    void testInvalidPatternTrailingBackslash()
    {
        test_router r;
        BOOST_TEST_THROWS(
            r.add(http::method::get, "/path\\", noop),
            std::exception);
    }

    //--------------------------------------------
    // Router Options Tests
    //--------------------------------------------

    void testCaseSensitiveMatch()
    {
        bool handler_called = false;

        test_router r(router_options().case_sensitive(true));
        r.add(http::method::get, "/Api",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Exact case - should match
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/Api"), req));
            BOOST_TEST(handler_called);
        }

        // Wrong case - should not match
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api"), req));
            BOOST_TEST(!handler_called);
        }
    }

    void testCaseInsensitiveMatch()
    {
        bool handler_called = false;

        test_router r(router_options().case_sensitive(false));
        r.add(http::method::get, "/Api",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Different case - should match
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api"), req));
            BOOST_TEST(handler_called);
        }

        // Upper case - should match
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/API"), req));
            BOOST_TEST(handler_called);
        }
    }

    void testStrictModeNoTrailingSlash()
    {
        bool handler_called = false;

        test_router r(router_options().strict(true));
        r.add(http::method::get, "/api/users",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Without trailing slash - should match
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api/users"), req));
            BOOST_TEST(handler_called);
        }

        // With trailing slash - should not match in strict mode
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api/users/"), req));
            BOOST_TEST(!handler_called);
        }
    }

    void testNonStrictModeTrailingSlash()
    {
        bool handler_called = false;

        test_router r(router_options().strict(false));
        r.add(http::method::get, "/api/users",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Without trailing slash - should match
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api/users"), req));
            BOOST_TEST(handler_called);
        }

        // With trailing slash - should also match in non-strict mode
        {
            params req;
            handler_called = false;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/api/users/"), req));
            BOOST_TEST(handler_called);
        }
    }

    //--------------------------------------------
    // Escape Sequence Tests
    //--------------------------------------------

    void testEscapedColon()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/path\\:literal",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/path:literal"), req));
        BOOST_TEST(handler_called);
        BOOST_TEST_EQ(req.params.size(), 0u);
    }

    void testEscapedAsterisk()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/path\\*star",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/path*star"), req));
        BOOST_TEST(handler_called);
    }

    void testEscapedBrace()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/path\\{brace\\}",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Use percent-encoded braces: { = %7B, } = %7D
        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/path%7Bbrace%7D"), req));
        BOOST_TEST(handler_called);
    }

    void testEscapedBackslash()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/path\\\\slash",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/path%5Cslash"), req));
        BOOST_TEST(handler_called);
    }

    //--------------------------------------------
    // Quoted Name Tests
    //--------------------------------------------

    void testQuotedParamName()
    {
        std::string captured_value;

        test_router r;
        r.add(http::method::get, "/items/:\"with spaces\"",
            [&](params& p) -> route_task
            {
                captured_value = get_param(p, "with spaces");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/items/123"), req));
        BOOST_TEST_EQ(captured_value, "123");
    }

    void testQuotedWildcardName()
    {
        std::string captured_value;

        test_router r;
        r.add(http::method::get, "/files/*\"file-path\"",
            [&](params& p) -> route_task
            {
                captured_value = get_param(p, "file-path");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/files/a/b/c.txt"), req));
        BOOST_TEST_EQ(captured_value, "a/b/c.txt");
    }

    //--------------------------------------------
    // Edge Case Tests
    //--------------------------------------------

    void testAdjacentParamsWithSeparator()
    {
        std::string from_val, to_val;

        test_router r;
        r.add(http::method::get, "/:from-:to",
            [&](params& p) -> route_task
            {
                from_val = get_param(p, "from");
                to_val = get_param(p, "to");
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/LAX-JFK"), req));
        BOOST_TEST_EQ(from_val, "LAX");
        BOOST_TEST_EQ(to_val, "JFK");
    }

    void testManyParams()
    {
        test_router r;
        r.add(http::method::get, "/:a/:b/:c/:d/:e/:f/:g/:h/:i/:j",
            [](params& p) -> route_task
            {
                BOOST_TEST_EQ(p.params.size(), 10u);
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/1/2/3/4/5/6/7/8/9/10"), req));
        BOOST_TEST_EQ(req.params.size(), 10u);
        BOOST_TEST_EQ(get_param(req, "a"), "1");
        BOOST_TEST_EQ(get_param(req, "j"), "10");
    }

    void testConsecutiveSlashes()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/a//b",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/a//b"), req));
        BOOST_TEST(handler_called);
    }

    void testRootPath()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/"), req));
        BOOST_TEST(handler_called);
    }

    void testNestedOptionalGroups()
    {
        int call_count = 0;

        test_router r;
        r.add(http::method::get, "/a{/b{/c}}",
            [&](params&) -> route_task
            {
                ++call_count;
                co_return route_done;
            });

        // All levels
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/a/b/c"), req));
            BOOST_TEST_EQ(call_count, 1);
        }

        // Two levels
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/a/b"), req));
            BOOST_TEST_EQ(call_count, 2);
        }

        // One level
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/a"), req));
            BOOST_TEST_EQ(call_count, 3);
        }
    }

    void testMultipleOptionalGroups()
    {
        int call_count = 0;

        test_router r;
        r.add(http::method::get, "{/a}{/b}",
            [&](params&) -> route_task
            {
                ++call_count;
                co_return route_done;
            });

        // Both groups
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/a/b"), req));
            BOOST_TEST_EQ(call_count, 1);
        }

        // First only
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/a"), req));
            BOOST_TEST_EQ(call_count, 2);
        }

        // Second only
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view("/b"), req));
            BOOST_TEST_EQ(call_count, 3);
        }

        // Neither
        {
            params req;
            capy::test::run_blocking()(r.dispatch(
                http::method::get, urls::url_view(""), req));
            BOOST_TEST_EQ(call_count, 4);
        }
    }

    void testParamMustNotBeEmpty()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/users/:id/posts",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Empty param value - should not match
        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/users//posts"), req));
        BOOST_TEST(!handler_called);
    }

    void testWildcardMustNotBeEmpty()
    {
        bool handler_called = false;

        test_router r;
        r.add(http::method::get, "/files/*path",
            [&](params&) -> route_task
            {
                handler_called = true;
                co_return route_done;
            });

        // Empty wildcard value - should not match
        params req;
        capy::test::run_blocking()(r.dispatch(
            http::method::get, urls::url_view("/files/"), req));
        BOOST_TEST(!handler_called);
    }

    void run()
    {
        testCopyConstruction();
        testCopyAssignment();
        testDefaultConstruction();
        testOptionsHandler();
        testExplicitOptionsPriority();
        testAllMethodsHandler();
        testOptionsStarGlobal();

        // Route pattern integration tests
        testParamCapture();
        testMultipleParams();
        testWildcardCapture();
        testOptionalGroup();
        testParamWithDash();
        testNoMatch();
        testGitHubStyleRoute();
        testFileExtensionGroup();
        testParamsClearedBetweenRequests();
        testUrlEncodedParam();

        // Path adjustment tests
        testPathAdjustmentSimple();
        testPathAdjustmentNested2Levels();
        testPathAdjustmentNested3Levels();
        testPathAdjustmentNested4Levels();
        testPathAdjustmentWithRoute();
        testPathAdjustmentLongUrl();

        // Invalid pattern tests
        testInvalidPatternMissingParamName();
        testInvalidPatternMissingWildcardName();
        testInvalidPatternUnclosedGroup();
        testInvalidPatternUnexpectedCloseBrace();
        testInvalidPatternUnterminatedQuote();
        testInvalidPatternEmptyQuotedName();
        testInvalidPatternReservedChars();
        testInvalidPatternTrailingBackslash();

        // Router options tests
        testCaseSensitiveMatch();
        testCaseInsensitiveMatch();
        testStrictModeNoTrailingSlash();
        testNonStrictModeTrailingSlash();

        // Escape sequence tests
        testEscapedColon();
        testEscapedAsterisk();
        testEscapedBrace();
        testEscapedBackslash();

        // Quoted name tests
        testQuotedParamName();
        testQuotedWildcardName();

        // Edge case tests
        testAdjacentParamsWithSeparator();
        testManyParams();
        testConsecutiveSlashes();
        testRootPath();
        testNestedOptionalGroups();
        testMultipleOptionalGroups();
        testParamMustNotBeEmpty();
        testWildcardMustNotBeEmpty();
    }
};

TEST_SUITE(
    any_router_test,
    "boost.http.server.any_router");

} // http
} // boost
