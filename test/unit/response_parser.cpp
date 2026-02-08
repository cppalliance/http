//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/response_parser.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

class response_parser_test
{
public:
    void
    testSpecial()
    {
        // response_parser() - default constructor (no state)
        {
            BOOST_TEST_NO_THROW(response_parser());
        }

        // operator=(response_parser&&)
        {
            response_parser pr;
            auto cfg = make_parser_config(parser_config{false});
            BOOST_TEST_NO_THROW(pr = response_parser(cfg));
        }
        {
            response_parser pr;
            BOOST_TEST_NO_THROW(pr = response_parser());
        }

        // response_parser(cfg)
        {
            auto cfg = make_parser_config(parser_config{false});
            response_parser pr(cfg);
        }

        // response_parser(response_parser&&)
        {
            auto cfg = make_parser_config(parser_config{false});
            response_parser pr1(cfg);
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
    "boost.http.response_parser");

} // http
} // boost
