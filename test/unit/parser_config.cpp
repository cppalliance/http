//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2024 Christian Mazakas
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/parser_config.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http_proto {

struct parser_config_test
{
    void run()
    {
    }
};

TEST_SUITE(
    parser_config_test,
    "boost.http_proto.parser_config");

} // http_proto
} // boost
