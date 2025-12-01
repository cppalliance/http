//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_SERVER_CORS_HPP
#define BOOST_HTTP_PROTO_SERVER_CORS_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/server/route_handler.hpp>
#include <boost/http_proto/status.hpp>
#include <chrono>

namespace boost {
namespace http_proto {

struct cors_options
{
    std::string origin;
    std::string methods;
    std::string allowedHeaders;
    std::string exposedHeaders;
    std::chrono::seconds max_age{ 0 };
    status result = status::no_content;
    bool preFligthContinue = false;
    bool credentials = false;
};

class cors
{
public:
    BOOST_HTTP_PROTO_DECL
    explicit cors(
        cors_options options = {}) noexcept;

    BOOST_HTTP_PROTO_DECL
    route_result
    operator()(
        Request& req,
        Response& res) const;

private:
    cors_options options_;
};

} // http_proto
} // boost

#endif
