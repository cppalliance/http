//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ROUTER_HPP
#define BOOST_HTTP_SERVER_ROUTER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/basic_router.hpp>
#include <boost/http/server/router_types.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/write.hpp>
#include <boost/capy/io/any_buffer_source.hpp>
#include <boost/capy/io/any_buffer_sink.hpp>
#include <boost/http/datastore.hpp>
#include <boost/http/request.hpp>           // VFALCO forward declare?
#include <boost/http/request_parser.hpp>    // VFALCO forward declare?
#include <boost/http/response.hpp>          // VFALCO forward declare?
#include <boost/http/serializer.hpp>        // VFALCO forward declare?
#include <boost/url/url_view.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <span>

namespace boost {
namespace http {

/** The coroutine task type returned by route handlers.

    Route handlers are coroutines that process HTTP requests and
    must return this type. The underlying @ref route_result
    (a `system::error_code`) communicates the routing disposition
    back to the router:

    @li Return @ref route::next to decline handling and allow
        subsequent handlers in the same route to process the request.

    @li Return @ref route::next_route to skip remaining handlers
        in the current route and proceed to the next matching route.

    @li Return a non-failing code (one for which
        `error_code::failed()` returns `false`) to indicate the
        response is complete. The connection will either close
        or proceed to the next request.

    @li Return a failing error code to signal an error that
        prevents normal processing.

    @par Example
    @code
    // A handler that serves static files
    route_task serve_file(route_params& p)
    {
        auto path = find_file(p.path);
        if(path.empty())
            co_return route::next;  // Not found, try next handler

        p.res.set_body_file(path);
        co_return {};  // Success
    }

    // A handler that requires authentication
    route_task require_auth(route_params& p)
    {
        if(! p.session_data.contains<user_session>())
        {
            p.status(http::status::unauthorized);
            co_return {};
        }
        co_return route::next;  // Authenticated, continue chain
    }
    @endcode

    @see @ref route_result, @ref route, @ref route_params
*/
using route_task = capy::task<route_result>;

//-----------------------------------------------

/** Parameters object for HTTP route handlers
*/
struct BOOST_HTTP_SYMBOL_VISIBLE
    route_params : route_params_base
{
    urls::url_view url; // The complete request target
    http::request req;
    http::response res;
    capy::any_buffer_source req_body;
    capy::any_buffer_sink res_body;
    http::request_parser parser;
    http::serializer serializer;
    http::datastore route_data; // arbitrary data
    http::datastore session_data;

    BOOST_HTTP_DECL ~route_params();
    BOOST_HTTP_DECL void reset(); // reset per request
    BOOST_HTTP_DECL route_params& status(http::status code);

    BOOST_HTTP_DECL route_task send(std::string_view body);
};

/** The default router type using @ref route_params.
*/
using router = basic_router<route_params>;

} // http
} // boost

#endif
