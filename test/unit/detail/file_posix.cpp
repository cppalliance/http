//
// Copyright (c) 2022 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/detail/file_posix.hpp>

#if BOOST_HTTP_USE_POSIX_FILE

#include "file_test.hpp"

namespace boost {
namespace http {
namespace detail {

class file_posix_test
{
public:
    void
    run()
    {
        test_file<file_posix>();
    }
};

TEST_SUITE(
    file_posix_test,
    "boost.http.detail.file_posix");

} // detail
} // http
} // boost

#endif
