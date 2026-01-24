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
#include <boost/capy/buffers/buffer_param.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/http/datastore.hpp>
#include <boost/capy/task.hpp>
#include <boost/http/request.hpp>           // VFALCO forward declare?
#include <boost/http/request_parser.hpp>    // VFALCO forward declare?
#include <boost/http/response.hpp>          // VFALCO forward declare?
#include <boost/http/serializer.hpp>        // VFALCO forward declare?
#include <boost/url/url_view.hpp>
#include <boost/system/error_code.hpp>
#include <memory>

namespace boost {
namespace http {

struct acceptor_config
{
    bool is_ssl;
    bool is_admin;
};

//-----------------------------------------------

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
    /** The complete request target

        This is the parsed directly from the start
        line contained in the HTTP request and is
        never modified.
    */
    urls::url_view url;

    /** The HTTP request message
    */
    http::request req;

    /** The HTTP response message
    */
    http::response res;

    /** The HTTP request parser
        This can be used to take over reading the body.
    */
    http::request_parser parser;

    /** The HTTP response serializer
    */
    http::serializer serializer;

    /** A container for storing arbitrary data associated with the request.
        This starts out empty for each new request.
    */
    http::datastore route_data;

    /** A container for storing arbitrary data associated with the session.

        This starts out empty for each new session.
    */
    http::datastore session_data;

    /** Destructor
    */
    BOOST_HTTP_DECL
    ~route_params();

    /** Reset the object for a new request.
        This clears any state associated with
        the previous request, preparing the object
        for use with a new request.
    */
    BOOST_HTTP_DECL
    void reset();

    /** Set the status code of the response.
        @par Example
        @code
        res.status( http::status::not_found );
        @endcode
        @param code The status code to set.
        @return A reference to this response.
    */
    BOOST_HTTP_DECL
    route_params&
    status(http::status code);

    /** Send the HTTP response with the given body.

        This convenience coroutine handles the entire response
        lifecycle in a single call, similar to Express.js
        `res.send()`. It performs the following steps:

        @li Sets the response body to the provided string.
        @li Sets the `Content-Length` header automatically.
        @li If `Content-Type` is not already set, detects the
            type: bodies starting with `<` are sent as
            `text/html; charset=utf-8`, otherwise as
            `text/plain; charset=utf-8`.
        @li Serializes and transmits the complete response.

        After calling this function the response is complete.
        Do not attempt to modify or send additional data.

        @par Example
        @code
        // Plain text (no leading '<')
        route_task hello( route_params& rp )
        {
            co_return co_await rp.send( "Hello, World!" );
        }

        // HTML (starts with '<')
        route_task greeting( route_params& rp )
        {
            co_return co_await rp.send( "<h1>Welcome</h1>" );
        }

        // Explicit Content-Type for JSON
        route_task api( route_params& rp )
        {
            rp.res.set( http::field::content_type, "application/json" );
            co_return co_await rp.send( R"({"status":"ok"})" );
        }
        @endcode

        @param body The content to send as the response body.

        @return A @ref route_task that completes when the response
        has been fully transmitted, yielding a @ref route_result
        indicating success or failure.
    */
    route_task send(std::string_view body)
    {
        if(! res.exists(http::field::content_type))
        {
            if(! body.empty() && body[0] == '<')
                res.set(http::field::content_type,
                    "text/html; charset=utf-8");
            else
                res.set(http::field::content_type,
                    "text/plain; charset=utf-8");
        }

        if(! res.exists(http::field::content_length))
            res.set_payload_size(body.size());

        co_await write(capy::make_buffer(body));
        co_return co_await end();
    }

    /** Write buffer data to the response body.

        This coroutine writes the provided buffer sequence to
        the response output stream. It is used for streaming
        responses where the body is sent in chunks.

        The response headers must be set appropriately before
        calling this function (e.g., set Transfer-Encoding to
        chunked, or set Content-Length if known).

        @par Example
        @code
        route_task stream_response( route_params& rp )
        {
            rp.res.set( field::transfer_encoding, "chunked" );

            // Write in chunks
            std::string chunk1 = "Hello, ";
            co_await rp.write( capy::make_buffer(chunk1) );

            std::string chunk2 = "World!";
            co_await rp.write( capy::make_buffer(chunk2) );

            co_return co_await rp.end();
        }
        @endcode

        @param buffers A buffer sequence containing the data to write.

        @return A @ref route_task that completes when the write
        operation is finished.
    */
    route_task
    write(capy::ConstBufferSequence auto const& buffers)
    {
        return write_impl(capy::make_const_buffer_param(buffers));
    }

    /** Complete a streaming response.

        This coroutine finalizes a streaming response that was
        started with @ref write calls. For chunked transfers,
        it sends the final chunk terminator.

        @par Example
        @code
        route_task send_file( route_params& rp )
        {
            rp.res.set( field::transfer_encoding, "chunked" );

            // Stream file contents...
            while( ! file.eof() )
            {
                auto data = file.read();
                co_await rp.write( capy::make_buffer(data) );
            }

            co_return co_await rp.end();
        }
        @endcode

        @return A @ref route_task that completes when the response
        has been fully finalized.
    */
    virtual route_task end() = 0;

protected:
    /** Implementation of write with type-erased buffers.

        Derived classes must implement this to perform the
        actual write operation.

        @param buffers Type-erased buffer sequence.

        @return A task that completes when the write is done.
    */
    virtual route_task write_impl(
        capy::const_buffer_param buffers) = 0;
};

/** The default router type using @ref route_params.
*/
using router = basic_router<route_params>;

} // http
} // boost

#endif
