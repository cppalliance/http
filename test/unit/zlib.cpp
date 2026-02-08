//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/capy/ex/execution_context.hpp>
#include <boost/http/zlib.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

class test_context : public capy::execution_context
{
public:
    ~test_context()
    {
        shutdown();
        destroy();
    }
};

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
        test_context ctx;
        zlib::install_deflate_service(ctx);
    }

    void
    test_inflate()
    {
        test_context ctx;
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
