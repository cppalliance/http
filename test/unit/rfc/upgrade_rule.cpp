//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/rfc/upgrade_rule.hpp>

#include "test_rule.hpp"

namespace boost {
namespace http {

struct upgrade_rule_test
{
    void
    run()
    {
        ok(upgrade_rule, "x");
        ok(upgrade_rule, "xyz");
        ok(upgrade_rule, "xyz/1");
        ok(upgrade_rule, "xyz/1, abc/2");
        bad(upgrade_rule, "");
        bad(upgrade_rule, "/");
        bad(upgrade_rule, "", grammar::error::mismatch);
    }
};

TEST_SUITE(
    upgrade_rule_test,
    "boost.http.upgrade_rule");

} // http
} // boost
