//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/rfc/token_rule.hpp>

#include "test_rule.hpp"

namespace boost {
namespace http {

struct token_rule_test
{
    void
    run()
    {
        ok(token_rule, "x", "x");
        ok(token_rule, "xyz", "xyz");
        bad(token_rule, "", grammar::error::need_more);
    }
};

TEST_SUITE(
    token_rule_test,
    "boost.http.token_rule");

} // http
} // boost
