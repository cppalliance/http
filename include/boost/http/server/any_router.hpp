//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ANY_ROUTER_HPP
#define BOOST_HTTP_SERVER_ANY_ROUTER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/router.hpp>
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

template<class> class basic_router;

/** A type-erased router for dispatching HTTP requests.

    `any_router` is the non-template base class for all routers.
    It holds a shared reference to an internal routing table
    that is built incrementally as routes are added. The routing
    table uses contiguous flat arrays for cache-friendly dispatch.

    Copies of an `any_router` share the same underlying routing
    data. Modifying a router after it has been copied is not
    permitted and results in undefined behavior.

    @par Thread Safety

    `dispatch` may be called concurrently on routers that share
    the same data. Modification through `basic_router` is not
    thread-safe and must not be performed concurrently with any
    other operation.

    @see basic_router, router
*/
class BOOST_HTTP_DECL
    any_router
{
    struct impl;
    std::shared_ptr<impl> impl_;

    template<class> friend class basic_router;

protected:
    using opt_flags = unsigned int;

    enum
    {
        is_invalid = 0,
        is_plain = 1,
        is_error = 2,
        is_exception = 8
    };

    struct BOOST_HTTP_DECL
        handler
    {
        char const kind;
        explicit handler(char kind_) noexcept : kind(kind_) {}
        virtual ~handler() = default;
        virtual auto invoke(route_params&) const ->
            route_task = 0;
    };

    using handler_ptr = std::unique_ptr<handler>;

    struct handlers
    {
        std::size_t n;
        handler_ptr* p;
    };

    struct BOOST_HTTP_DECL
        options_handler
    {
        virtual ~options_handler() = default;
        virtual route_task invoke(
            route_params&,
            std::string_view allow) const = 0;
    };

    using options_handler_ptr = std::unique_ptr<options_handler>;

protected:
    using match_result = route_params::match_result;
    struct matcher;
    struct entry;

    // Construct with options
    explicit any_router(opt_flags);

    // Registration helpers
    void add_middleware(std::string_view pattern, handlers hn);
    void inline_router(std::string_view pattern, any_router&& sub);
    std::size_t new_route(std::string_view pattern);
    void add_to_route(std::size_t idx, http::method verb, handlers hn);
    void add_to_route(std::size_t idx, std::string_view verb, handlers hn);
    void finalize_pending();
    void set_options_handler_impl(options_handler_ptr p);

public:
    /** Default constructor.

        Creates a router in an empty state. The only valid
        operations on a default-constructed router are
        assignment, destruction, and copying.
    */
    any_router() = default;

    any_router(any_router const&) = default;
    any_router(any_router&&) noexcept = default;
    any_router& operator=(any_router const&) = default;
    any_router& operator=(any_router&&) noexcept = default;
    ~any_router() = default;

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
        route_params& p) const;

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
        route_params& p) const;

    /** Maximum nesting depth for routers.

        This limit applies to nested routers added via use().
        Exceeding this limit throws std::length_error at
        insertion time.
    */
    static constexpr std::size_t max_path_depth = 16;
};

} // http
} // boost

#endif
