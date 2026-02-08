//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/rfc/list_rule.hpp>

#include <boost/http/rfc/token_rule.hpp>
#include <boost/url/grammar/parse.hpp>

#include "test_suite.hpp"

#include <initializer_list>

namespace boost {
namespace http {

struct list_rule_test
{
    void
    bad(core::string_view s)
    {
        auto rv = grammar::parse(s,
            list_rule(token_rule));
        BOOST_TEST(rv.has_error());
    }

    void
    ok( core::string_view s,
        std::initializer_list<
            core::string_view> init)
    {
        auto rv = grammar::parse(s,
            list_rule(token_rule));
        if(! BOOST_TEST(rv.has_value()))
            return;
        auto const& t = *rv;
        if(! BOOST_TEST(
                t.size() == init.size()))
            return;
        auto it = t.begin();
        for(std::size_t i = 0;
                i < t.size(); ++i)
            BOOST_TEST(*it++ ==
                init.begin()[i]);
    }

    void
    testParse()
    {
        bad(" ");
        bad("\t");
        bad(" \t");
        bad("     ");

        ok("", {});
        ok("x", {"x"});
        ok("x,y", {"x","y"});

        ok("", {});
        ok(",", {});
        bad(", ");
        ok(", ,", {});
        ok(",,,", {});

        ok("1", {"1"});
        ok(",1", {"1"});
        ok("1,", {"1"});
        ok(", 1", {"1"});
        ok("1 ,", {"1"});

        ok("1,2", {"1", "2"});
        ok("1,2", {"1", "2"});

        ok("1,2,3", {"1", "2", "3"});
        ok(", 1,\t2, 3", {"1", "2", "3"});
    }

    void
    run()
    {
        testParse();
    }
};

TEST_SUITE(
    list_rule_test,
    "boost.http.list_rule");

} // http
} // boost
