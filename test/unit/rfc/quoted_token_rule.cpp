//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/rfc/quoted_token_rule.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http_proto {

struct quoted_token_rule_test
{
    void
    bad(core::string_view s)
    {
        http_proto::bad(quoted_token_rule, s);
    }

    void
    ok(
        core::string_view s,
        core::string_view r,
        std::size_t escapes = 0)
    {
        auto rv = grammar::parse(
            s, quoted_token_rule);
        if(! BOOST_TEST(rv.has_value()))
            return;
        BOOST_TEST(rv.value() == r);
        BOOST_TEST(
            rv->unescaped_size() == rv->size() - escapes);
    }

    void
    run()
    {
        // token
        bad("");
        bad(" ");
        bad("a b");
        ok("x", "x");
        ok(
            "!#$%&'*+-.^_`|~"
            "0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
            "!#$%&'*+-.^_`|~"
            "0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

        // quoted-string
        bad(R"(")");
        bad(R"("" )");
        bad(R"( "")");
        bad(R"(""")");
        bad(R"("\")");
        ok(R"("")", R"()");
        ok(R"("x")", R"(x)");
        ok(R"("\\")", R"(\\)", 1);
        ok(R"("abc\ def")", R"(abc\ def)", 1);
        ok(R"("a\"b\"c")", R"(a\"b\"c)", 2);
    }
};

TEST_SUITE(
    quoted_token_rule_test,
    "boost.http_proto.quoted_token_rule");

} // http_proto
} // boost
