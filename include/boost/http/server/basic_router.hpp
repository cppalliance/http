//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_BASIC_ROUTER_HPP
#define BOOST_HTTP_SERVER_BASIC_ROUTER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/router_types.hpp>
#include <boost/http/server/detail/router_base.hpp>
#include <boost/http/method.hpp>
#include <boost/url/url_view.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/capy/io_task.hpp>
#include <boost/assert.hpp>
#include <exception>
#include <string_view>
#include <type_traits>

namespace boost {
namespace http {

template<class> class basic_router;

/** Configuration options for HTTP routers.
*/
struct router_options
{
    /** Constructor.

        Routers constructed with default options inherit the values of
        @ref case_sensitive and @ref strict from the parent router.
        If there is no parent, both default to `false`.
        The value of @ref merge_params always defaults to `false`
        and is never inherited.
    */
    router_options() = default;

    /** Set whether to merge parameters from parent routers.

        This setting controls whether route parameters defined on parent
        routers are made available in nested routers. It is not inherited
        and always defaults to `false`.

        @par Example
        @code
        basic_router r( router_options()
            .merge_params( true )
            .case_sensitive( true )
            .strict( false ) );
        @endcode

        @param value `true` to merge parameters from parent routers.

        @return A reference to `*this` for chaining.
    */
    router_options&
    merge_params(
        bool value) noexcept
    {
        v_ = (v_ & ~1) | (value ? 1 : 0);
        return *this;
    }

    /** Set whether pattern matching is case-sensitive.

        When this option is not set explicitly, the value is inherited
        from the parent router or defaults to `false` if there is no parent.

        @par Example
        @code
        basic_router r( router_options()
            .case_sensitive( true )
            .strict( true ) );
        @endcode

        @param value `true` to perform case-sensitive path matching.

        @return A reference to `*this` for chaining.
    */
    router_options&
    case_sensitive(
        bool value) noexcept
    {
        if(value)
            v_ = (v_ & ~6) | 2;
        else
            v_ = (v_ & ~6) | 4;
        return *this;
    }

    /** Set whether pattern matching is strict.

        When this option is not set explicitly, the value is inherited
        from the parent router or defaults to `false` if there is no parent.
        Strict matching treats a trailing slash as significant:
        the pattern `"/api"` matches `"/api"` but not `"/api/"`.
        When strict matching is disabled, these paths are treated
        as equivalent.

        @par Example
        @code
        basic_router r( router_options()
            .strict( true )
            .case_sensitive( false ) );
        @endcode

        @param value `true` to enable strict path matching.

        @return A reference to `*this` for chaining.
    */
    router_options&
    strict(
        bool value) noexcept
    {
        if(value)
            v_ = (v_ & ~24) | 8;
        else
            v_ = (v_ & ~24) | 16;
        return *this;
    }

private:
    template<class> friend class basic_router;
    unsigned int v_ = 0;
};

//-----------------------------------------------

/** A container for HTTP route handlers.

    `basic_router` objects store and dispatch route handlers based on the
    HTTP method and path of an incoming request. Routes are added with a
    path pattern, method, and an associated handler, and the router is then
    used to dispatch the appropriate handler.

    Patterns used to create route definitions have percent-decoding applied
    when handlers are mounted. A literal "%2F" in the pattern string is
    indistinguishable from a literal '/'. For example, "/x%2Fz" is the
    same as "/x/z" when used as a pattern.

    @par Example
    @code
    using router_type = basic_router<route_params>;
    router_type router;
    router.get( "/hello",
        []( route_params& p )
        {
            p.res.status( status::ok );
            p.res.set_body( "Hello, world!" );
            return route::send;
        } );
    @endcode

    Router objects are lightweight, shared references to their contents.
    Copies of a router obtained through construction, conversion, or
    assignment do not create new instances; they all refer to the same
    underlying data.

    @par Handlers

    Regular handlers are invoked for matching routes and have this
    equivalent signature:
    @code
    route_result handler( Params& p )
    @endcode

    The return value is a @ref route_result used to indicate the desired
    action through @ref route enum values, or to indicate that a failure
    occurred. Failures are represented by error codes for which
    `system::error_code::failed()` returns `true`.

    When a failing error code is produced and remains unhandled, the
    router enters error-dispatching mode. In this mode, only error
    handlers are invoked. Error handlers are registered globally or
    for specific paths and execute in the order of registration whenever
    a failing error code is present in the response.

    Error handlers have this equivalent signature:
    @code
    route_result error_handler( Params& p, system::error_code ec )
    @endcode

    Each error handler may return any failing @ref system::error_code,
    which is equivalent to calling:
    @code
    p.next( ec ); // with ec == true
    @endcode

    Returning @ref route::next indicates that control should proceed to
    the next matching error handler. Returning a different failing code
    replaces the current error and continues dispatch in error mode using
    that new code. Error handlers are invoked until one returns a result
    other than @ref route::next.

    Exception handlers have this equivalent signature:
    @code
    route_result exception_handler( Params& p, E ex )
    @endcode

    Where `E` is the type of exception caught. Handlers installed for an
    exception of type `E` will also be called when the exception type is
    a derived class of `E`. Exception handlers are invoked in the order
    of registration whenever an exception is present in the request.

    The prefix match is not strict: middleware attached to `"/api"`
    will also match `"/api/users"` and `"/api/data"`. When registered
    before route handlers for the same prefix, middleware runs before
    those routes. This is analogous to `app.use( path, ... )` in
    Express.js.

    @par Thread Safety

    Member functions marked `const` such as @ref dispatch and @ref resume
    may be called concurrently on routers that refer to the same data.
    Modification of routers through calls to non-`const` member functions
    is not thread-safe and must not be performed concurrently with any
    other member function.

    @par Nesting Depth

    Routers may be nested to a maximum depth of `max_path_depth` (16 levels).
    Exceeding this limit throws `std::length_error` when the nested router
    is added via @ref use. This limit ensures that @ref flat_router dispatch
    never overflows its fixed-size tracking arrays.

    @par Constraints

    `Params` must be publicly derived from @ref route_params_base.

    @tparam Params The type of the parameters object passed to handlers.
*/
template<class P>
class basic_router : public detail::router_base
{
    static_assert(std::derived_from<P, route_params_base>);

    template<class T>
    static inline constexpr char handler_kind =
        []() -> char
        {
            if constexpr (detail::returns_route_task<T, P&>)
            {
                return is_plain;
            }
            else if constexpr (detail::returns_route_task<
                T, P&, system::error_code>)
            {
                return is_error;
            }
            else if constexpr(
                std::is_base_of_v<router_base, T> &&
                std::is_convertible_v<T const volatile*,
                    router_base const volatile*> &&
                std::is_constructible_v<T, basic_router<P>>)
            {
                return is_router;        
            }
            else if constexpr (detail::returns_route_task<
                T, P&, std::exception_ptr>)
            {
                return is_exception;
            }
            else
            {
                return is_invalid;
            }
        }();

    template<class... Ts>
    static inline constexpr bool handler_crvals =
        ((!std::is_lvalue_reference_v<Ts> || 
        std::is_const_v<std::remove_reference_t<Ts>> ||
        std::is_function_v<std::remove_reference_t<Ts>>) && ...);
        
    template<char Mask, class... Ts>
    static inline constexpr bool handler_check = 
        (((handler_kind<Ts> & Mask) != 0) && ...);

    template<class H>
    struct handler_impl : handler
    {  
        std::decay_t<H> h;

        template<class H_>
        explicit handler_impl(H_ h_)
            : handler(handler_kind<H>)
            , h(std::forward<H_>(h_))
        {
        }
    
        auto invoke(route_params_base& rp) const ->
            capy::io_task<> override
        {
            if constexpr (detail::returns_route_task<H, P&>)
            {
                return h(static_cast<P&>(rp));
            }
            else if constexpr (detail::returns_route_task<
                H, P&, system::error_code>)
            {
                return h(static_cast<P&>(rp), rp.ec_);
            }
            else if constexpr (detail::returns_route_task<
                H, P&, std::exception_ptr>)
            {
                return h(static_cast<P&>(rp), rp.ep_);
            }
            else
            {
                // impossible with flat router
                std::terminate();
            }
        }

        detail::router_base*
        get_router() noexcept override
        {
            if constexpr (std::is_base_of_v<
                detail::router_base, std::decay_t<H>>)
                return &h;
            else
                return nullptr;
        }
    };

    template<class H>
    static handler_ptr make_handler(H&& h)
    {
        return std::make_unique<handler_impl<H>>(std::forward<H>(h));
    }

    template<std::size_t N>
    struct handlers_impl : handlers
    {
        handler_ptr v[N];

        template<class... HN>
        explicit handlers_impl(HN&&... hn)
        {
            p = v;
            n = sizeof...(HN);
            assign<0>(std::forward<HN>(hn)...);
        }

    private:
        template<std::size_t I, class H1, class... HN>
        void assign(H1&& h1, HN&&... hn)
        {
            v[I] = make_handler(std::forward<H1>(h1));
            assign<I+1>(std::forward<HN>(hn)...);
        }

        template<std::size_t>
        void assign(int = 0)
        {
        }
    };

    template<class... HN>
    static auto make_handlers(HN&&... hn)
    {
        return handlers_impl<sizeof...(HN)>(
            std::forward<HN>(hn)...);
    }

public:
    /** The type of params used in handlers.
    */
    using params_type = P;

    /** A fluent interface for defining handlers on a specific route.

        This type represents a single route within the router and
        provides a chainable API for registering handlers associated
        with particular HTTP methods or for all methods collectively.

        Typical usage registers one or more handlers for a route:
        @code
        router.route( "/users/:id" )
            .get( show_user )
            .put( update_user )
            .all( log_access );
        @endcode

        Each call appends handlers in registration order.
    */
    class fluent_route;

    basic_router(basic_router const&) = delete;
    basic_router& operator=(basic_router const&) = delete;

    /** Constructor.

        Creates an empty router with the specified configuration.
        Routers constructed with default options inherit the values
        of @ref router_options::case_sensitive and
        @ref router_options::strict from the parent router, or default
        to `false` if there is no parent. The value of
        @ref router_options::merge_params defaults to `false` and
        is never inherited.

        @param options The configuration options to use.
    */
    explicit
    basic_router(
        router_options options = {})
        : router_base(options.v_)
    {
    }

    /** Construct a router from another router with compatible types.

        This constructs a router that shares the same underlying routing
        state as another router whose params type is a base class of `Params`.

        The resulting router participates in shared ownership of the
        implementation; copying the router does not duplicate routes or
        handlers, and changes visible through one router are visible
        through all routers that share the same underlying state.

        @par Constraints

        `Params` must be derived from `OtherParams`.

        @param other The router to construct from.

        @tparam OtherParams The params type of the source router.
    */
    template<class OtherP>
        requires std::derived_from<OtherP, P>
    basic_router(
        basic_router<OtherP>&& other) noexcept
        : router_base(std::move(other))
    {
    }

    /** Add middleware handlers for a path prefix.

        Each handler registered with this function participates in the
        routing and error-dispatch process for requests whose path begins
        with the specified prefix, as described in the @ref basic_router
        class documentation. Handlers execute in the order they are added
        and may return @ref route::next to transfer control to the
        subsequent handler in the chain.

        @par Example
        @code
        router.use( "/api",
            []( route_params& p )
            {
                if( ! authenticate( p ) )
                {
                    p.res.status( 401 );
                    p.res.set_body( "Unauthorized" );
                    return route::send;
                }
                return route::next;
            },
            []( route_params& p )
            {
                p.res.set_header( "X-Powered-By", "MyServer" );
                return route::next;
            } );
        @endcode

        @par Preconditions

        @p pattern must be a valid path prefix; it may be empty to
        indicate the root scope.

        @param pattern The pattern to match.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.
    */
    template<class H1, class... HN>
    void use(
        std::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        static_assert(handler_crvals<H1, HN...>,
            "pass handlers by value or std::move()");
        static_assert(! handler_check<8, H1, HN...>,
            "cannot use exception handlers here");
        static_assert(handler_check<7, H1, HN...>,
            "invalid handler signature");
        add_impl(pattern, make_handlers(
            std::forward<H1>(h1), std::forward<HN>(hn)...));
    }

    /** Add global middleware handlers.

        Each handler registered with this function participates in the
        routing and error-dispatch process as described in the
        @ref basic_router class documentation. Handlers execute in the
        order they are added and may return @ref route::next to transfer
        control to the next handler in the chain.

        This is equivalent to writing:
        @code
        use( "/", h1, hn... );
        @endcode

        @par Example
        @code
        router.use(
            []( Params& p )
            {
                p.res.erase( "X-Powered-By" );
                return route::next;
            } );
        @endcode

        @par Constraints

        @p h1 must not be convertible to @ref std::string_view.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.
    */
    template<class H1, class... HN>
    void use(H1&& h1, HN&&... hn)
        requires (!std::convertible_to<H1, std::string_view>)
    {
        static_assert(handler_crvals<H1, HN...>,
            "pass handlers by value or std::move()");
        static_assert(! handler_check<8, H1, HN...>,
            "cannot use exception handlers here");
        static_assert(handler_check<7, H1, HN...>,
            "invalid handler signature");
        use(std::string_view(),
            std::forward<H1>(h1), std::forward<HN>(hn)...);
    }

    /** Add exception handlers for a route pattern.

        Registers one or more exception handlers that will be invoked
        when an exception is thrown during request processing for routes
        matching the specified pattern.

        Handlers are invoked in the order provided until one handles
        the exception.

        @par Example
        @code
        app.except( "/api*",
            []( route_params& p, std::exception const& ex )
            {
                p.res.set_status( 500 );
                return route::send;
            } );
        @endcode

        @param pattern The route pattern to match, or empty to match
        all routes.

        @param h1 The first exception handler.

        @param hn Additional exception handlers.
    */
    template<class H1, class... HN>
    void except(
        std::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        static_assert(handler_crvals<H1, HN...>,
            "pass handlers by value or std::move()");
        static_assert(handler_check<8, H1, HN...>,
            "only exception handlers are allowed here");
        add_impl(pattern, make_handlers(
            std::forward<H1>(h1), std::forward<HN>(hn)...));
    }

    /** Add global exception handlers.

        Registers one or more exception handlers that will be invoked
        when an exception is thrown during request processing for any
        route.

        Equivalent to calling `except( "", h1, hn... )`.

        @par Example
        @code
        app.except(
            []( route_params& p, std::exception const& ex )
            {
                p.res.set_status( 500 );
                return route::send;
            } );
        @endcode

        @param h1 The first exception handler.

        @param hn Additional exception handlers.
    */
    template<class H1, class... HN>
    void except(H1&& h1, HN&&... hn)
        requires (!std::convertible_to<H1, std::string_view>)
    {
        static_assert(handler_crvals<H1, HN...>,
            "pass handlers by value or std::move()");
        static_assert(handler_check<8, H1, HN...>,
            "only exception handlers are allowed here");
        except(std::string_view(),
            std::forward<H1>(h1), std::forward<HN>(hn)...);
    }

    /** Add handlers for all HTTP methods matching a path pattern.

        This registers regular handlers for the specified path pattern,
        participating in dispatch as described in the @ref basic_router
        class documentation. Handlers run when the route matches,
        regardless of HTTP method, and execute in registration order.
        Error handlers and routers cannot be passed here. A new route
        object is created even if the pattern already exists.

        @par Example
        @code
        router.route( "/status" )
            .add( method::head, check_headers )
            .add( method::get, send_status )
            .all( log_access );
        @endcode

        @par Preconditions

        @p pattern must be a valid path pattern; it must not be empty.

        @param pattern The path pattern to match.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.
    */
    template<class H1, class... HN>
    void all(
        std::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        static_assert(handler_crvals<H1, HN...>,
            "pass handlers by value or std::move()");
        static_assert(handler_check<1, H1, HN...>,
            "only normal route handlers are allowed here");
        this->route(pattern).all(
            std::forward<H1>(h1), std::forward<HN>(hn)...);
    }

    /** Add route handlers for a method and pattern.

        This registers regular handlers for the specified HTTP verb and
        path pattern, participating in dispatch as described in the
        @ref basic_router class documentation. Error handlers and
        routers cannot be passed here.

        @param verb The known HTTP method to match.

        @param pattern The path pattern to match.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.
    */
    template<class H1, class... HN>
    void add(
        http::method verb,
        std::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        static_assert(handler_crvals<H1, HN...>,
            "pass handlers by value or std::move()");
        static_assert(handler_check<1, H1, HN...>,
            "only normal route handlers are allowed here");
        this->route(pattern).add(verb,
            std::forward<H1>(h1), std::forward<HN>(hn)...);
    }

    /** Add route handlers for a method string and pattern.

        This registers regular handlers for the specified HTTP verb and
        path pattern, participating in dispatch as described in the
        @ref basic_router class documentation. Error handlers and
        routers cannot be passed here.

        @param verb The HTTP method string to match.

        @param pattern The path pattern to match.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.
    */
    template<class H1, class... HN>
    void add(
        std::string_view verb,
        std::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        static_assert(handler_crvals<H1, HN...>,
            "pass handlers by value or std::move()");
        static_assert(handler_check<1, H1, HN...>,
            "only normal route handlers are allowed here");
        this->route(pattern).add(verb,
            std::forward<H1>(h1), std::forward<HN>(hn)...);
    }

    /** Return a fluent route for the specified path pattern.

        Adds a new route to the router for the given pattern.
        A new route object is always created, even if another
        route with the same pattern already exists. The returned
        @ref fluent_route reference allows method-specific handler
        registration (such as GET or POST) or catch-all handlers
        with @ref fluent_route::all.

        @param pattern The path expression to match against request
        targets. This may include parameters or wildcards following
        the router's pattern syntax. May not be empty.

        @return A fluent route interface for chaining handler
        registrations.
    */
    auto
    route(
        std::string_view pattern) -> fluent_route
    {
        return fluent_route(*this, pattern);
    }
};

template<class P>
class basic_router<P>::
    fluent_route
{
public:
    fluent_route(fluent_route const&) = default;

    /** Add handlers that apply to all HTTP methods.

        This registers regular handlers that run for any request matching
        the route's pattern, regardless of HTTP method. Handlers are
        appended to the route's handler sequence and are invoked in
        registration order whenever a preceding handler returns
        @ref route::next. Error handlers and routers cannot be passed here.

        This function returns a @ref fluent_route, allowing additional
        method registrations to be chained. For example:
        @code
        router.route( "/resource" )
            .all( log_request )
            .add( method::get, show_resource )
            .add( method::post, update_resource );
        @endcode

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.

        @return A reference to `*this` for chained registrations.
    */
    template<class H1, class... HN>
    auto all(
        H1&& h1, HN&&... hn) ->
            fluent_route
    {
        static_assert(handler_check<1, H1, HN...>);
        owner_.add_impl(owner_.get_layer(layer_idx_), std::string_view{},
            owner_.make_handlers(
                std::forward<H1>(h1), std::forward<HN>(hn)...));
        return *this;
    }

    /** Add handlers for a specific HTTP method.

        This registers regular handlers for the given method on the
        current route, participating in dispatch as described in the
        @ref basic_router class documentation. Handlers are appended
        to the route's handler sequence and invoked in registration
        order whenever a preceding handler returns @ref route::next.
        Error handlers and routers cannot be passed here.

        @param verb The HTTP method to match.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.

        @return A reference to `*this` for chained registrations.
    */
    template<class H1, class... HN>
    auto add(
        http::method verb,
        H1&& h1, HN&&... hn) ->
            fluent_route
    {
        static_assert(handler_check<1, H1, HN...>);
        owner_.add_impl(owner_.get_layer(layer_idx_), verb, owner_.make_handlers(
            std::forward<H1>(h1), std::forward<HN>(hn)...));
        return *this;
    }

    /** Add handlers for a method string.

        This registers regular handlers for the given HTTP method string
        on the current route, participating in dispatch as described in
        the @ref basic_router class documentation. This overload is
        intended for methods not represented by @ref http::method.
        Handlers are appended to the route's handler sequence and invoked
        in registration order whenever a preceding handler returns
        @ref route::next. Error handlers and routers cannot be passed here.

        @param verb The HTTP method string to match.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.

        @return A reference to `*this` for chained registrations.
    */
    template<class H1, class... HN>
    auto add(
        std::string_view verb,
        H1&& h1, HN&&... hn) ->
            fluent_route
    {
        static_assert(handler_check<1, H1, HN...>);
        owner_.add_impl(owner_.get_layer(layer_idx_), verb, owner_.make_handlers(
            std::forward<H1>(h1), std::forward<HN>(hn)...));
        return *this;
    }

private:
    friend class basic_router;
    fluent_route(
        basic_router& owner,
        std::string_view pattern)
        : layer_idx_(owner.new_layer_idx(pattern))
        , owner_(owner)
    {
    }

    std::size_t layer_idx_;
    basic_router& owner_;
};

} // http
} // boost

#endif
