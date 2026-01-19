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
#include <boost/capy/datastore.hpp>
#include <boost/capy/task.hpp>
#include <boost/http/request.hpp>  // VFALCO forward declare?
#include <boost/http/request_parser.hpp>  // VFALCO forward declare?
#include <boost/http/response.hpp>        // VFALCO forward declare?
#include <boost/http/serializer.hpp>      // VFALCO forward declare?
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

    //http::route_task capy::task<http::route_result>
};

} // http
} // boost

#endif
