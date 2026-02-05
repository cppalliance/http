//
// Copyright (c) 2026 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/http_worker.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

struct http_worker_test
{
    void
    run()
    {
    }
};

TEST_SUITE(
    http_worker_test,
    "boost.http.http_worker");

} // http
} // boost
