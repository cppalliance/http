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
#include <boost/capy/io_task.hpp>
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

/** Parameters object for HTTP route handlers.

    This structure holds all the context needed for a route
    handler to process an HTTP request and generate a response.

    @par Example
    @code
    route_task my_handler(route_params& p)
    {
        p.res.set(field::content_type, "text/plain");
        co_await p.send("Hello, World!");
        co_return route_done;
    }
    @endcode

    @see route_task, route_result
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

    BOOST_HTTP_DECL capy::io_task<> send(std::string_view body = {});
};

/** The default router type using @ref route_params.
*/
using router = basic_router<route_params>;

} // http
} // boost

#endif
