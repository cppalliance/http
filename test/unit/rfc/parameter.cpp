//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/rfc/parameter.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

struct parameter_test
{
    void
    run()
    {
    }
};

TEST_SUITE(
    parameter_test,
    "boost.http.parameter");

} // http
} // boost
