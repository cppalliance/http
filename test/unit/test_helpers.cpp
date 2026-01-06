//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "test_helpers.hpp"

#include <boost/http/fields.hpp>
#include <algorithm>

namespace boost {
namespace http {

void
test_fields(
    fields_base const& f,
    core::string_view match)
{
    fields r(match);
    std::size_t n = std::distance(
        f.begin(), f.end());
    BOOST_TEST_EQ(f.size(), n);
    auto it0 = r.begin();
    auto it1 = f.begin();
    auto const end = r.end();
    while(it0 != end)
    {
        if(! BOOST_TEST_NE(
                it1, f.end()))
            break;
        BOOST_TEST_EQ(
            it0->name, it1->name);
        BOOST_TEST_EQ(
            it0->value, it1->value);
        ++it0;
        ++it1;
    }
}

} // http
} // boost

