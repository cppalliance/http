//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ROUTE_HANDLER_HPP
#define BOOST_HTTP_SERVER_ROUTE_HANDLER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/router_types.hpp>
#include <boost/capy/make_affine.hpp>
#include <boost/capy/datastore.hpp>
#include <boost/capy/task.hpp>
#include <boost/http/request.hpp>  // VFALCO forward declare?
#include <boost/http/request_parser.hpp>  // VFALCO forward declare?
#include <boost/http/response.hpp>        // VFALCO forward declare?
#include <boost/http/serializer.hpp>      // VFALCO forward declare?
#include <boost/url/url_view.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>

namespace boost {
namespace http {

struct acceptor_config
{
    bool is_ssl;
    bool is_admin;
};

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
    capy::datastore route_data;

    /** A container for storing arbitrary data associated with the session.

        This starts out empty for each new session.
    */
    capy::datastore session_data;

    /** The suspender for this session.

        This can be used to suepend from the router and resume routing later.
    */
    suspender suspend;

    /** Executor associated with the session.
    */
    capy::any_dispatcher ex;

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

    BOOST_HTTP_DECL
    route_params&
    set_body(std::string s);

    /** Read the request body and receive a value.

        This function reads the entire request body into the specified sink.
        When the read operation completes, the given callback is invoked with
        the result.

        The @p callback parameter must be a function object with this
        equivalent signature, where `T` is the type produced by the value sink:
        @code
        void ( T&& t );
        @endcode

        @par Example
        @code
        rp.read_body(
            capy::string_body_sink(),
            []( std::string s )
            {
                // body read successfully
            });
        @endcode

        If an error or an exception occurs during the read, it is propagated
        through the router to the next error or exception handler.

        @param sink The body sink to read into.
        @param callback The function to call when the read completes.
        @return The route result, which must be returned immediately
            from the route handler.
    */
    template<
        class ValueSink,
        class Callback>
    auto
    read_body(
        ValueSink&& sink,
        Callback&& callback) ->
            route_result;

#ifdef BOOST_HTTP_HAS_CORO

    /** Spawn a coroutine for this route.

        This function is used to spawn a coroutine
        for the route handler. The coroutine is
        passed a reference to the route_params object,
        and when it returns, the returned route_result
        is returned from this function.

        @par Example
        @code
        auto handler =
            []( route_params& rp ) -> route_result
            {
                return rp.spawn(
                    []( route_params& rp ) -> capy::task<route_result>
                    {
                        co_return route_result::next;
                    });
            };
        @endcode
        @param coro The coroutine to spawn.
        @return The route result, which must be returned immediately
            from the route handler.
    */
    BOOST_HTTP_DECL
    auto spawn(
        capy::task<route_result> coro) ->
            route_result;

#endif

protected:
    std::function<void(void)> finish_;
};

//-----------------------------------------------

template<
    class ValueSink,
    class Callback>
auto
route_params::
read_body(
    ValueSink&& sink,
    Callback&& callback) ->
        route_result
{
    using T = typename std::decay<ValueSink>::type;

    struct on_finish
    {
        T& sink;
        resumer resume;
        typename std::decay<Callback>::type cb;

        on_finish(
            T& sink_,
            resumer resume_,
            Callback&& cb_) 
            : sink(sink_)
            , resume(resume_)
            , cb(std::forward<Callback>(cb_))
        {
        }

        void operator()()
        {
            resume(std::move(cb)(sink.release()));
        }
    };

    return suspend(
        [&](resumer resume)
        {
            finish_ = on_finish(
                this->parser.set_body<T>(
                    std::forward<ValueSink>(sink)),
                resume,
                std::forward<Callback>(callback));
        });
}

//-----------------------------------------------

#ifdef BOOST_HTTP_HAS_CORO

/** Create a route handler from a coroutine function

    This is a convenience function for creating
    route handlers from coroutine functions.

    @par Signature
    The coroutine function must have this signature:
    @code
    capy::task<route_result>( route_params& rp );
    @endcode

    @param f The coroutine function to invoke.
    @return A route handler object.
*/
inline
auto
co_route(std::function<
    capy::task<route_result>(route_params&)> f)
{
    return
        [f_ = std::move(f)]( route_params& rp )
        {
            return rp.spawn(f_(rp));
        };
}

#endif

} // http
} // boost

#endif
