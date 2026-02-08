//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#if 0

// Test that header file is self-contained.
#include <boost/http/rfc/chunk_ext_rule.hpp>

#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/range.hpp>

#include "test_suite.hpp"

#include <initializer_list>

namespace boost {
namespace http {

static
bool
operator==(
    chunk_ext_rule::reference const& lhs,
    chunk_ext_rule::reference const& rhs) noexcept
{
    return
        lhs.name == rhs.name &&
        lhs.value == rhs.value;
}

class chunk_ext_rule_test
{
public:
    void
    bad(
        string_view s)
    {
        error_code ec;
        auto it = s.data();
        auto const end = it + s.size();
        chunk_ext_rule::reference t;
        BOOST_TEST(
            ! chunk_ext_rule::parse(
                it, end, ec, t));
    }

    void
    good(
        string_view s,
        string_view k,
        string_view v = {})
    {
        error_code ec;
        auto it = s.data();
        auto const end = it + s.size();
        chunk_ext_rule::reference t;
        if(! BOOST_TEST(
            chunk_ext_rule::parse(
                it, end, ec, t)))
            return;
        BOOST_TEST(t.name == k);
        BOOST_TEST(t.value == v);
    }

    void
    good(
        string_view s,
        std::initializer_list<
            chunk_ext_rule::reference> init)
    {
        grammar::range<
            chunk_ext_rule> t;
        error_code ec;
        auto it = s.data();
        auto const end = it + s.size();
        if(! BOOST_TEST(grammar::parse(
            it, end, ec, t)))
            return;
        if(! BOOST_TEST(
            t.size() == init.size()))
            return;
        BOOST_TEST(std::equal(
            t.begin(), t.end(), init.begin()));
    }

    void
    testParse()
    {
        good(";a", "a");
        good(";a", "a");
        good(";ab", "ab");
        good(";ab=c", "ab", "c");
        good(";ab=cd", "ab", "cd");
        good(";ab=\"cd\"", "ab", "\"cd\"");

        // BWS
        good(" ;a", "a");
        good("; a", "a");
        good(";a =c", "a", "c");
        good(";a= c", "a", "c");
        good(" ; a = c", "a", "c");
        good(" ;a=\"c\"", "a", "\"c\"");
        good("; a=\"c\"", "a", "\"c\"");
        good(";a =\"c\"", "a", "\"c\"");
        good(";a= \"c\"", "a", "\"c\"");
        good(" ; a = \"c\"", "a", "\"c\"");

        bad("");
        bad(";");
        bad(";;");
        bad(";ab=");
    }

    void
    testRange()
    {
        good(";a", {{"a",""}});
        good(";a;b", {{"a",""}, {"b",""}});
        good(";a;b;c", {{"a",""}, {"b",""}, {"c",""}});
        good(";a=de;b=\"x\";c", {{"a","de"}, {"b","\"x\""}, {"c",""}});
    }

    void
    run()
    {
        testParse();
        testRange();
    }
};

TEST_SUITE(
    chunk_ext_rule_test,
    "boost.http.chunk_ext_rule");

} // http
} // boost

#endif
