//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_JSON_JSON_BODY_HPP
#define BOOST_HTTP_JSON_JSON_BODY_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/route_handler.hpp>
#include <boost/json/parse_options.hpp>
#include <boost/json/storage_ptr.hpp>

namespace boost {
namespace http {

/** Options for the JSON body middleware.
*/
struct json_body_options
{
    /// Storage used for parsed values.
    json::storage_ptr storage;

    /// Options controlling JSON parsing behavior.
    json::parse_options parse_opts;
};

/** Route handler middleware that parses a JSON request body.

    This middleware checks that the request is a POST with
    Content-Type `application/json`, reads the entire request
    body into a @ref json_sink, and stores the parsed
    `json::value` in the route's @ref datastore under the
    key `json::value`. If the request does not match, the
    middleware yields to the next handler.

    @par Example
    @code
    router<route_params> r;
    r.use( json_body() );
    r.post( "/api/data",
        []( route_params& p ) -> route_task
        {
            auto& jv = p.route_data.get<json::value>();
            // use jv...
            co_return route_done;
        } );
    @endcode

    @see json_body_options
*/
class BOOST_HTTP_DECL json_body
{
    json_body_options options_;

public:
    /** Construct with default options.
    */
    explicit json_body(json_body_options options = {}) noexcept;

    /** Handle a request.

        Parses the JSON request body and stores the result
        in @ref route_params::route_data.

        @param p The route parameters for the current request.

        @return A @ref route_task that completes with
        @ref route_next on success, or an error on parse failure.
    */
    route_task operator()(route_params& p) const;
};

} // namespace http
} // namespace boost

#endif
