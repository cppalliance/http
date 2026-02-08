//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/rfc/quoted_token_rule.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

struct quoted_token_rule_test
{
    void
    run()
    {
        auto const& t = quoted_token_rule;

        // token
        bad(t, "");
        ok(t, "x");
        ok(t,
            "!#$%&'*+-.^_`|~"
            "0123456789"
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        bad(t, "a b");

        // quoted-string
        ok(t, "\"\"");
        ok(t, "\"x\"");
        ok(t, "\"\\,\"");
        ok(t, "\"abc\\ def\"");
    }
};

TEST_SUITE(
    quoted_token_rule_test,
    "boost.http.quoted_token_rule");

} // http
} // boost
