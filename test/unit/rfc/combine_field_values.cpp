//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/rfc/combine_field_values.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

struct combine_field_values_test
{
    void
    run()
    {
        fields f(
            "Content-Length: 42\r\n"
            "x: 1\r\n"
            "y: 2\r\n"
            "Set-Cookie: a\r\n"
            "x: 3\r\n"
            "z: 4\r\n"
            "Set-Cookie: b\r\n"
            "x: 5\r\n"
            "p: 6\r\n"
            "User-Agent: boost\r\n"
            "\r\n");
        grammar::recycled_ptr<
            std::string> temp;
        BOOST_TEST(combine_field_values(
            f.find_all("x"), temp) ==
            "1,3,5");
        BOOST_TEST(combine_field_values(
            f.find_all("y"), temp) ==
            "2");
        BOOST_TEST(combine_field_values(
            f.find_all("q"), temp) == "");
        BOOST_TEST(combine_field_values(
            f.find_all(
                field::user_agent), temp) == "boost");
        BOOST_TEST(combine_field_values(
            f.find_all(
                field::set_cookie), temp) == "a,b");
    }
};

TEST_SUITE(
    combine_field_values_test,
    "boost.http.combine_field_values");

} // http
} // boost
