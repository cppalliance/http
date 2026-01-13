//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "test_suite.hpp"

namespace boost {
namespace http {

/*
    Four body styles for `parser`
        * in-place
        * a DynamicBuffer
        * a Sink
        * a parser::stream

    Four body styles for `serializer`
        * Specify a ConstBufferSequence
        * Specify a Source
        * Write into a serializer::stream
        * in-place

    struct half_duplex_client;
    struct full_duplex_client;
    struct pipelined_client;
    struct websocket_client;
    struct websocket_client_with_permessage_deflate;

    struct half_duplex_server;
    struct full_duplex_server;
    struct pipelined_server;
    struct websocket_server;
    struct websocket_server_with_permessage_deflate;

*/

struct sandbox_test
{
    void
    run()
    {
/*
        std::string s;
        read_header( sock, pr );
        char temp[1024];
        auto mb = capy::make_buffer(temp);
        while(! pr.is_complete())
        {
            auto[ec, n] = co_await
                async_read_some( sock, pr, mb );
        }
*/
    }
};

TEST_SUITE(
    sandbox_test,
    "boost.http.sandbox");

} // http
} // boost
