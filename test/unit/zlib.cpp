//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/core/polystore.hpp>
#include <boost/http/zlib.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

struct zlib_test
{
    void
    test_error_code()
    {
        // TODO
        boost::system::error_code ec{ zlib::error::buf_err };
    }

    void
    test_deflate()
    {
        // TODO
        http::polystore ctx;
        zlib::install_deflate_service(ctx);
    }

    void
    test_inflate()
    {
        // TODO
        http::polystore ctx;
        zlib::install_inflate_service(ctx);
    }

    void
    run()
    {
        test_error_code();
    #ifdef BOOST_HTTP_HAS_ZLIB
        test_deflate();
        test_inflate();
    #endif
    }
};

TEST_SUITE(zlib_test, "boost.http.zlib");

} // namespace http
} // namespace boost
