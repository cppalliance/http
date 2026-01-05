//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/rfc/content_disposition_rule.hpp>

#include <boost/url/grammar/parse.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http_proto {

struct content_disposition_rule_test
{
    void
    bad(core::string_view s)
    {
        auto rv = grammar::parse(
            s, content_disposition_rule);
        BOOST_TEST(rv.has_error());
    }

    void
    ok(
        core::string_view s,
        core::string_view type,
        std::initializer_list<
            std::pair<core::string_view, core::string_view>> init)
    {
        auto rv = grammar::parse(
            s, content_disposition_rule);
        if(! BOOST_TEST(rv.has_value()))
            return;
        auto const& cd = *rv;
        BOOST_TEST(cd.type == type);
        if(! BOOST_TEST(
                cd.params.size() == init.size()))
            return;
        auto it = cd.params.begin();
        for(std::size_t i = 0; i < init.size(); ++i)
        {
            auto param = *it++;
            BOOST_TEST(
                param.name == init.begin()[i].first);
            BOOST_TEST(
                param.value == init.begin()[i].second);
        }
    }

    void
    testParse()
    {
        bad("");
        bad(" ");
        bad(" inline");
        bad("inline ");
        bad(";");
        bad("inline;");
        bad("inline; ");
        bad("inline; ;");
        bad("inline; a");
        bad("inline; a=");
        bad("inline; =b");
        bad("inline; a=b;");

        ok("inline;a=b", "inline", {{"a", "b"}});
        ok("inline;a=b;c=d", "inline", {{"a", "b"}, {"c", "d"}});
        ok("inline; a=b", "inline", {{"a", "b"}});
        ok("inline ;a=b; c=d", "inline", {{"a", "b"}, {"c", "d"}});
        ok("inline;  a=b", "inline", {{"a", "b"}});
        ok("inline;   a=b", "inline", {{"a", "b"}});
        ok("inline ;a=b", "inline", {{"a", "b"}});
        ok("inline ; a=b", "inline", {{"a", "b"}});
        ok("inline  ;  a=b", "inline", {{"a", "b"}});
    }

    void
    run()
    {
        testParse();
    }
};

TEST_SUITE(
    content_disposition_rule_test,
    "boost.http_proto.content_disposition_rule");

} // http_proto
} // boost
