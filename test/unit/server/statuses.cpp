//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/statuses.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct statuses_test
{
    void
    test_is_empty()
    {
        // Empty body status codes
        BOOST_TEST( statuses::is_empty( 204 ) );
        BOOST_TEST( statuses::is_empty( 205 ) );
        BOOST_TEST( statuses::is_empty( 304 ) );

        // Non-empty status codes
        BOOST_TEST( ! statuses::is_empty( 200 ) );
        BOOST_TEST( ! statuses::is_empty( 404 ) );
        BOOST_TEST( ! statuses::is_empty( 500 ) );
    }

    void
    test_is_redirect()
    {
        // Redirect status codes
        BOOST_TEST( statuses::is_redirect( 300 ) );
        BOOST_TEST( statuses::is_redirect( 301 ) );
        BOOST_TEST( statuses::is_redirect( 302 ) );
        BOOST_TEST( statuses::is_redirect( 303 ) );
        BOOST_TEST( statuses::is_redirect( 307 ) );
        BOOST_TEST( statuses::is_redirect( 308 ) );

        // Not redirects
        BOOST_TEST( ! statuses::is_redirect( 200 ) );
        BOOST_TEST( ! statuses::is_redirect( 304 ) ); // Not Modified is not a redirect
        BOOST_TEST( ! statuses::is_redirect( 404 ) );
    }

    void
    test_is_retry()
    {
        // Retryable status codes
        BOOST_TEST( statuses::is_retry( 502 ) );
        BOOST_TEST( statuses::is_retry( 503 ) );
        BOOST_TEST( statuses::is_retry( 504 ) );

        // Not retryable
        BOOST_TEST( ! statuses::is_retry( 200 ) );
        BOOST_TEST( ! statuses::is_retry( 500 ) );
        BOOST_TEST( ! statuses::is_retry( 501 ) );
    }

    void
    run()
    {
        test_is_empty();
        test_is_redirect();
        test_is_retry();
    }
};

TEST_SUITE(
    statuses_test,
    "boost.http.server.statuses");

} // http
} // boost
