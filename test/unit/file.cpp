//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/file.hpp>

#include <system_error>
#include "file_test.hpp"

namespace boost {
namespace http {

struct file_test
{
    void
    testThrowingOverloads()
    {
        // constructor
        BOOST_TEST_THROWS(
            file("missing.txt", file_mode::scan),
            std::system_error);

        file f;
        char buf[1];

        BOOST_TEST_THROWS(
            f.open("missing.txt", file_mode::scan),
            std::system_error);
        // BOOST_TEST_THROWS(
        //     f.close(),
        //     std::system_error);
        BOOST_TEST_THROWS(
            f.size(),
            std::system_error);
        BOOST_TEST_THROWS(
            f.pos(),
            std::system_error);
        BOOST_TEST_THROWS(
            f.seek(1),
            std::system_error);
        BOOST_TEST_THROWS(
            f.read(buf, 1),
            std::system_error);
        BOOST_TEST_THROWS(
            f.write(buf, 1),
            std::system_error);
    }

    void
    run()
    {
        test_file<file, true>();
        testThrowingOverloads();
    }
};

TEST_SUITE(
    file_test,
    "boost.http.file");

} // http
} // boost
