//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/multipart_form_sink.hpp>

#include "test_suite.hpp"

#include <boost/filesystem/operations.hpp>

namespace boost {
namespace http_proto {

struct multipart_form_sink_test
{
    void run()
    {
        core::string_view cs =
            "---X\r\n"
            "Content-Disposition: form-data; name=\"username\"\r\n"
            "\r\n"
            "alice\r\n"
            // "---X\r\n"
            // "Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n"
            // "Content-Type: text/plain\r\n"
            // "\r\n"
            // "Hello world!\r\n"
            "---X--\r\n";

        multipart_form_sink mfs1("-X");
        // multipart_form_sink mfs2("-X");
        auto rv = mfs1.write(buffers::const_buffer{ cs.data(), cs.size()}, false);
        BOOST_TEST(!rv.ec.failed());
        // for(auto& c : cs)
        // {
        //     mfs2.write(
        //         buffers::const_buffer{ &c, 1},
        //         &c != &cs[cs.size() - 1]);
        // }
    }
};

TEST_SUITE(
    multipart_form_sink_test,
    "boost.http_proto.multipart_form_sink");

} // http_proto
} // boost
