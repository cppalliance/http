//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_CORS_HPP
#define BOOST_HTTP_SERVER_CORS_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/route_handler.hpp>
#include <boost/http/status.hpp>
#include <chrono>

namespace boost {
namespace http {

struct cors_options
{
    std::string origin;
    std::string methods;
    std::string allowedHeaders;
    std::string exposedHeaders;
    std::chrono::seconds max_age{ 0 };
    status result = status::no_content;
    bool preFlightContinue = false;
    bool credentials = false;
};

class cors
{
public:
    BOOST_HTTP_DECL
    explicit cors(
        cors_options options = {}) noexcept;

    BOOST_HTTP_DECL
    route_result
    operator()(route_params& p) const;

private:
    cors_options options_;
};

} // http
} // boost

#endif
