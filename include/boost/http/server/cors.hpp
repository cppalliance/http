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
#include <boost/http/server/router.hpp>
#include <boost/http/status.hpp>
#include <chrono>

namespace boost {
namespace http {

/** Options for CORS middleware configuration.
*/
struct cors_options
{
    /// Allowed origin, or "*" for any. Empty defaults to "*".
    std::string origin;

    /// Allowed HTTP methods. Empty defaults to common methods.
    std::string methods;

    /// Allowed request headers.
    std::string allowedHeaders;

    /// Response headers exposed to client.
    std::string exposedHeaders;

    /// Max age for preflight cache.
    std::chrono::seconds max_age{ 0 };

    /// Status code for preflight response.
    status result = status::no_content;

    /// If true, pass preflight to next handler.
    bool preFlightContinue = false;

    /// If true, allow credentials.
    bool credentials = false;
};

/** CORS middleware for handling cross-origin requests.

    This middleware handles Cross-Origin Resource Sharing
    (CORS) by setting appropriate response headers and
    handling preflight OPTIONS requests.

    @par Example
    @code
    cors_options opts;
    opts.origin = "*";
    opts.methods = "GET,POST,PUT,DELETE";
    opts.credentials = true;

    router.use( cors( opts ) );
    @endcode

    @see cors_options
*/
class BOOST_HTTP_DECL cors
{
    cors_options options_;

public:
    /** Construct a CORS middleware.

        @param options Configuration options.
    */
    explicit cors(cors_options options = {}) noexcept;

    /** Handle a request.

        Sets CORS headers and handles preflight requests.

        @param rp The route parameters.

        @return A task that completes with the routing result.
    */
    route_task operator()(route_params& rp) const;
};

} // http
} // boost

#endif
