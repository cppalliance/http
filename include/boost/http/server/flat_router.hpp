//
// Copyright (c) 2026 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_FLAT_ROUTER_HPP
#define BOOST_HTTP_SERVER_FLAT_ROUTER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/router_types.hpp>
#include <boost/http/method.hpp>
#include <boost/url/url_view.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/assert.hpp>
#include <exception>
#include <memory>
#include <string_view>
#include <type_traits>

namespace boost {
namespace http {

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4251) // shared_ptr needs dll-interface
#endif

/** A flattened router optimized for dispatch performance.

    `flat_router` is constructed from a @ref router by flattening
    its nested structure into contiguous arrays. This eliminates
    pointer chasing during dispatch and improves cache locality.

    The dispatch algorithm uses fixed-size arrays sized by
    `detail::router_base::max_path_depth`. Since this limit is
    enforced when routers are nested, dispatch is guaranteed
    not to overflow.
*/
class BOOST_HTTP_DECL
    flat_router
{
    struct impl;
    std::shared_ptr<impl> impl_;

public:
    flat_router(
        detail::router_base&&);

    /** Dispatch a request using a known HTTP method.

        @param verb The HTTP method to match. Must not be
        @ref http::method::unknown.

        @param url The full request target used for route matching.

        @param p The params to pass to handlers.

        @return A task yielding the @ref route_result describing
        how routing completed.

        @throws std::invalid_argument If @p verb is
        @ref http::method::unknown.
    */
    route_task
    dispatch(
        http::method verb,
        urls::url_view const& url,
        route_params_base& p) const;

    /** Dispatch a request using a method string.

        @param verb The HTTP method string to match. Must not be empty.

        @param url The full request target used for route matching.

        @param p The params to pass to handlers.

        @return A task yielding the @ref route_result describing
        how routing completed.

        @throws std::invalid_argument If @p verb is empty.
    */
    route_task
    dispatch(
        std::string_view verb,
        urls::url_view const& url,
        route_params_base& p) const;
};

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

} // http
} // boost

#endif
