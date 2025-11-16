//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/response_parser.hpp>

#include <boost/rts/context.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http_proto {

class response_parser_test
{
public:
    void
    testSpecial()
    {
        // response_parser()
        {
            BOOST_TEST_NO_THROW(response_parser());
        }

        // operator=(response_parser&&)
        {
            response_parser pr;

            rts::context ctx;
            response_parser::config cfg;
            install_parser_service(ctx, cfg);

            BOOST_TEST_NO_THROW(pr = response_parser(ctx));
        }
        {
            response_parser pr;
            BOOST_TEST_NO_THROW(pr = response_parser());
        }

        // response_parser(rts::context&)
        {
            rts::context ctx;
            response_parser::config cfg;
            install_parser_service(ctx, cfg);
            response_parser pr(ctx);
        }

        // response_parser(response_parser&&)
        {
            rts::context ctx;
            install_parser_service(ctx, {});
            response_parser pr1(ctx);
            response_parser pr2(std::move(pr1));
        }
        {
            BOOST_TEST_NO_THROW(
                response_parser(response_parser()));
        }
    }

    void
    run()
    {
        testSpecial();
    }
};

TEST_SUITE(
    response_parser_test,
    "boost.http_proto.response_parser");

} // http_proto
} // boost
