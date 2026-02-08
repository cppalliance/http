//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/header_limits.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct header_limits_test
{
    void
    testSpecial()
    {
        // header_limits();
        {
            header_limits lim;
            (void)lim;
        }
    }

    void
    run()
    {
        testSpecial();
    }
};

TEST_SUITE(
    header_limits_test,
    "boost.http.header_limits");

} // http
} // boost
