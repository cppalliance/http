//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/rfc/quoted_token_view.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct quoted_token_view_test
{
    void
    run()
    {
    }
};

TEST_SUITE(
    quoted_token_view_test,
    "boost.http.quoted_token_view");

} // http
} // boost
