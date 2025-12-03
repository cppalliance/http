//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_SERVER_BASIC_ROUTER_HPP
#define BOOST_HTTP_PROTO_SERVER_BASIC_ROUTER_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/detail/type_traits.hpp>
#include <boost/http_proto/server/router_types.hpp>
#include <boost/http_proto/method.hpp>
#include <boost/capy/detail/call_traits.hpp> // VFALCO fix
#include <boost/core/detail/string_view.hpp>
#include <boost/url/url_view.hpp>
#include <boost/core/detail/static_assert.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/assert.hpp>
#include <type_traits>

namespace boost {
namespace http_proto {

template<class>
class basic_router;

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
        router r( router_options()
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
        router r( router_options()
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
        router r( router_options()
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
 
//namespace detail {

class any_router;

//-----------------------------------------------

// implementation for all routers
class any_router
{
private:
    template<class>
    friend class http_proto::basic_router;
    using opt_flags = unsigned int;

    struct BOOST_HTTP_PROTO_DECL any_handler
    {
        virtual ~any_handler() = default;
        virtual std::size_t count() const noexcept = 0;
        virtual route_result invoke(
            route_params_base&) const = 0;
    };

    using handler_ptr = std::unique_ptr<any_handler>;

    struct handler_list
    {
        std::size_t n;
        handler_ptr* p;
    };

    using match_result = route_params_base::match_result;
    struct matcher;
    struct layer;
    struct impl;

    BOOST_HTTP_PROTO_DECL ~any_router();
    BOOST_HTTP_PROTO_DECL any_router(opt_flags);
    BOOST_HTTP_PROTO_DECL any_router(any_router&&) noexcept;
    BOOST_HTTP_PROTO_DECL any_router(any_router const&) noexcept;
    BOOST_HTTP_PROTO_DECL any_router& operator=(any_router&&) noexcept;
    BOOST_HTTP_PROTO_DECL any_router& operator=(any_router const&) noexcept;
    BOOST_HTTP_PROTO_DECL std::size_t count() const noexcept;
    BOOST_HTTP_PROTO_DECL layer& new_layer(core::string_view pattern);
    BOOST_HTTP_PROTO_DECL void add_impl(core::string_view, handler_list const&);
    BOOST_HTTP_PROTO_DECL void add_impl(layer&,
        http_proto::method, handler_list const&);
    BOOST_HTTP_PROTO_DECL void add_impl(layer&,
        core::string_view, handler_list const&);
    BOOST_HTTP_PROTO_DECL route_result resume_impl(
        route_params_base&, route_result ec) const;
    BOOST_HTTP_PROTO_DECL route_result dispatch_impl(http_proto::method,
        core::string_view, urls::url_view const&,
            route_params_base&) const;
    BOOST_HTTP_PROTO_DECL route_result dispatch_impl(
        route_params_base&) const;
    route_result do_dispatch(route_params_base&) const;

    impl* impl_ = nullptr;
};

//} // detail

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
    p.next( ec ); // with ec.failed() == true
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

    @par Constraints

    `Params` must be publicly derived from @ref route_params_base.

    @tparam Params The type of the parameters object passed to handlers.
*/
template<class Params>
class basic_router : public /*detail::*/any_router
{
    // Params must be publicly derived from route_params_base
    BOOST_CORE_STATIC_ASSERT(
        detail::derived_from<route_params_base, Params>::value);

    // 0 = unrecognized
    // 1 = normal handler (Params&)
    // 2 = error handler  (Params&, error_code)
    // 4 = basic_router<Params>

    template<class T, class = void>
    struct handler_type
        : std::integral_constant<int, 0>
    {
    };

    // route_result( Params& ) const
    template<class T>
    struct handler_type<T, typename
        std::enable_if<std::is_convertible<
            decltype(std::declval<T const&>()(
                std::declval<Params&>())),
            route_result>::value
        >::type> : std::integral_constant<int, 1> {};

    // route_result( Params&, system::error_code const& ) const
    template<class T>
    struct handler_type<T, typename
        std::enable_if<std::is_convertible<
            decltype(std::declval<T const&>()(
                std::declval<Params&>(),
                std::declval<system::error_code const&>())),
            route_result>::value
        >::type> : std::integral_constant<int, 2> {};

    // basic_router<Params>
    template<class T>
    struct handler_type<T, typename
        std::enable_if<
            std::is_base_of<any_router, T>::value &&
            std::is_convertible<T const volatile*,
                any_router const volatile*>::value &&
            std::is_constructible<T, basic_router<Params>>::value
        >::type> : std::integral_constant<int, 4> {};

    template<std::size_t Mask, class... Ts>
    struct handler_check : std::true_type {};

    template<std::size_t Mask, class T0, class... Ts>
    struct handler_check<Mask, T0, Ts...>
        : std::conditional<
              ( (handler_type<T0>::value & Mask) != 0 ),
              handler_check<Mask, Ts...>,
              std::false_type
          >::type {};

    // exception handler (Params&, E)

    template<class H, class = void>
    struct except_type : std::false_type {};

    template<class H>
    struct except_type<H, typename std::enable_if<
        capy::detail::call_traits<H>{} && (
        mp11::mp_size<typename
            capy::detail::call_traits<H>::arg_types>{} == 2) &&
        std::is_convertible<Params&, mp11::mp_first<typename
            capy::detail::call_traits<H>::arg_types>>::value
            >::type>
        : std::true_type
    {
        // type of exception
        using type = typename std::decay<mp11::mp_second<typename
            capy::detail::call_traits<H>::arg_types>>::type;
    };

    template<class... Hs> using except_types =
        mp11::mp_all< except_type<Hs>... >;

public:
    /** The type of params used in handlers.
    */
    using params_type = Params;

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
        : any_router(options.v_)
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
    template<
        class OtherParams,
        class = typename std::enable_if<
            detail::derived_from<Params, OtherParams>::value>::type
    >
    basic_router(
        basic_router<OtherParams> const& other) noexcept
        : any_router(other)
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
        core::string_view pattern,
        H1&& h1, HN... hn)
    {
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler,
        // error handler, or router.
        BOOST_CORE_STATIC_ASSERT(handler_check<7, H1, HN...>::value);
        add_impl(pattern, make_handler_list(
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

        @p h1 must not be convertible to @ref core::string_view.

        @param h1 The first handler to add.

        @param hn Additional handlers to add, invoked after @p h1 in
        registration order.
    */
    template<class H1, class... HN
        , class = typename std::enable_if<
            ! std::is_convertible<H1, core::string_view>::value>::type>
    void use(H1&& h1, HN&&... hn)
    {
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler,
        // error handler, or router.
        BOOST_CORE_STATIC_ASSERT(handler_check<7, H1, HN...>::value);
        use(core::string_view(),
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
        core::string_view pattern,
        H1&& h1, HN... hn)
    {
        // If you get a compile error on this line it means that one or
        // more of the provided types is not a valid exception handler
        BOOST_CORE_STATIC_ASSERT(except_types<H1, HN...>::value);
        add_impl(pattern, make_except_list(
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
    template<class H1, class... HN
        , class = typename std::enable_if<
            ! std::is_convertible<H1, core::string_view>::value>::type>
    void except(H1&& h1, HN&&... hn)
    {
        // If you get a compile error on this line it means that one or
        // more of the provided types is not a valid exception handler
        BOOST_CORE_STATIC_ASSERT(except_types<H1, HN...>::value);
        except(core::string_view(),
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
        core::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler.
        // Error handlers and routers cannot be passed here.
        BOOST_CORE_STATIC_ASSERT(handler_check<1, H1, HN...>::value);
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
        http_proto::method verb,
        core::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler.
        // Error handlers and routers cannot be passed here.
        BOOST_CORE_STATIC_ASSERT(handler_check<1, H1, HN...>::value);
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
        core::string_view verb,
        core::string_view pattern,
        H1&& h1, HN&&... hn)
    {
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler.
        // Error handlers and routers cannot be passed here.
        BOOST_CORE_STATIC_ASSERT(handler_check<1, H1, HN...>::value);
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
        core::string_view pattern) -> fluent_route
    {
        return fluent_route(*this, pattern);
    }

    //--------------------------------------------

    /** Dispatch a request to the appropriate handler.

        This runs the routing and error-dispatch logic for the given HTTP
        method and target URL, as described in the @ref basic_router class
        documentation.

        @par Thread Safety

        This function may be called concurrently on the same object along
        with other `const` member functions. Each concurrent invocation
        must use distinct params objects.

        @param verb The HTTP method to match. This must not be
        @ref http_proto::method::unknown.

        @param url The full request target used for route matching.

        @param p The params to pass to handlers.

        @return The @ref route_result describing how routing completed.

        @throws std::invalid_argument If @p verb is
        @ref http_proto::method::unknown.
    */
    auto
    dispatch(
        http_proto::method verb,
        urls::url_view const& url,
        Params& p) const ->
            route_result
    {
        if(verb == http_proto::method::unknown)
            detail::throw_invalid_argument();
        return dispatch_impl(verb,
            core::string_view(), url, p);
    }

    /** Dispatch a request using a method string.

        This runs the routing and error-dispatch logic for the given HTTP
        method string and target URL, as described in the @ref basic_router
        class documentation. This overload is intended for method tokens
        that are not represented by @ref http_proto::method.

        @par Thread Safety

        This function may be called concurrently on the same object along
        with other `const` member functions. Each concurrent invocation
        must use distinct params objects.

        @param verb The HTTP method string to match. This must not
        be empty.

        @param url The full request target used for route matching.

        @param p The params to pass to handlers.

        @return The @ref route_result describing how routing completed.

        @throws std::invalid_argument If @p verb is empty.
    */
    auto
    dispatch(
        core::string_view verb,
        urls::url_view const& url,
        Params& p) ->
            route_result
    {
        // verb cannot be empty
        if(verb.empty())
            detail::throw_invalid_argument();
        return dispatch_impl(
            http_proto::method::unknown,
                verb, url, p);
    }

    /** Resume dispatch after a detached handler.

        This continues routing after a previous call to @ref dispatch
        returned @ref route::detach. It recreates the routing state and
        resumes as if the handler that detached had instead returned
        the specified @p rv from its body. The regular routing and
        error-dispatch logic then proceeds as described in the
        @ref basic_router class documentation. For example, if @p rv is
        @ref route::next, the next matching handlers are invoked.

        @par Thread Safety

        This function may be called concurrently on the same object along
        with other `const` member functions. Each concurrent invocation
        must use distinct params objects.

        @param p The params to pass to handlers.

        @param rv The @ref route_result to resume with, as if returned
        by the detached handler.

        @return The @ref route_result describing how routing completed.
    */
    auto
    resume(
        Params& p,
        route_result const& rv) const ->
            route_result
    {
         return resume_impl(p, rv);
    }

private:
    // used to avoid a race when modifying p.resume_
    struct set_resume
    {
        std::size_t& resume;
        bool cancel_ = true;

        ~set_resume()
        {
            if(cancel_)
                resume = 0;
        }

        set_resume(
            route_params_base& p) noexcept
            : resume(p.resume_)
        {
            resume = p.pos_;
        }

        void apply() noexcept
        {
            cancel_ = false;
        }
    };

    // wrapper for route handlers
    template<
        class H,
        class Ty = handler_type<typename
        std::decay<H>::type > >
    struct handler_impl : any_handler
    {
        typename std::decay<H>::type h;

        template<class... Args>
        explicit handler_impl(Args&&... args)
            : h(std::forward<Args>(args)...)
        {
        }

        std::size_t
        count() const noexcept override
        {
            return count(Ty{});
        }

        route_result
        invoke(
            route_params_base& p) const override
        {
            return invoke(static_cast<Params&>(p), Ty{});
        }

    private:
        std::size_t count(
            std::integral_constant<int, 0>) = delete;

        std::size_t count(
            std::integral_constant<int, 1>) const noexcept
        {
            return 1;
        }

        std::size_t count(
            std::integral_constant<int, 2>) const noexcept
        {
            return 1;
        }

        std::size_t count(
            std::integral_constant<int, 4>) const noexcept
        {
            return 1 + h.count();
        }

        route_result invoke(Params&,
            std::integral_constant<int, 0>) const = delete;

        // ( Params& )
        route_result invoke(Params& p,
            std::integral_constant<int, 1>) const
        {
            route_params_base& p_(p);
            if( p_.ec_.failed() ||
                p_.ep_)
                return http_proto::route::next;
            set_resume u(p_);
            auto rv = h(p);
            if(rv == http_proto::route::detach)
            {
                u.apply();
                return rv;
            }
            return rv;
        }

        // ( Params&, error_code )
        route_result
        invoke(Params& p,
            std::integral_constant<int, 2>) const
        {
            route_params_base& p_(p);
            if(! p_.ec_.failed())
                return http_proto::route::next;
            set_resume u(p_);
            auto rv = h(p, p_.ec_);
            if(rv == http_proto::route::detach)
            {
                u.apply();
                return rv;
            }
            return rv;
        }

        // any_router
        route_result invoke(Params& p,
            std::integral_constant<int, 4>) const
        {
            route_params_base& p_(p);
            if( p_.resume_ > 0 ||
                (   ! p_.ec_.failed() &&
                    ! p_.ep_))
                return h.dispatch_impl(p);
            return http_proto::route::next;
        }
    };

    template<class H, class E = typename
        except_type<typename std::decay<H>::type>::type>
    struct except_impl : any_handler
    {
        typename std::decay<H>::type h;

        template<class... Args>
        explicit except_impl(Args&&... args)
            : h(std::forward<Args>(args)...)
        {
        }

        std::size_t
        count() const noexcept override
        {
            return 1;
        }

        route_result
        invoke(Params& p) const override
        {
        #ifndef BOOST_NO_EXCEPTIONS
            route_params_base& p_(p);
            if(! p_.ep_)
                return http_proto::route::next;
            try
            {
                std::rethrow_exception(p_.ep_);
            }
            catch(E const& ex)
            {
                set_resume u(p_);
                // VFALCO What if h throws?
                auto rv = h(p, ex);
                if(rv == http_proto::route::detach)
                {
                    u.apply();
                    return rv;
                }
                return rv;
            }
            catch(...)
            {
                BOOST_ASSERT(p_.ep_);
                return http_proto::route::next;
            }
        #else
            (void)p;
            return http_proto::route::next;
        #endif
        }
    };

    template<std::size_t N>
    struct handler_list_impl : handler_list
    {
        template<class... HN>
        explicit handler_list_impl(HN&&... hn)
        {
            n = sizeof...(HN);
            p = v;
            assign<0>(std::forward<HN>(hn)...);
        }

        // exception handlers
        template<class... HN>
        explicit handler_list_impl(int, HN&&... hn)
        {
            n = sizeof...(HN);
            p = v;
            assign<0>(0, std::forward<HN>(hn)...);
        }

    private:
        template<std::size_t I, class H1, class... HN>
        void assign(H1&& h1, HN&&... hn)
        {
            v[I] = handler_ptr(new handler_impl<H1>(
                std::forward<H1>(h1)));
            assign<I+1>(std::forward<HN>(hn)...);
        }

        // exception handlers
        template<std::size_t I, class H1, class... HN>
        void assign(int, H1&& h1, HN&&... hn)
        {
            v[I] = handler_ptr(new except_impl<H1>(
                std::forward<H1>(h1)));
            assign<I+1>(0, std::forward<HN>(hn)...);
        }

        template<std::size_t>
        void assign(int = 0)
        {
        }

        handler_ptr v[N];
    };

    template<class... HN>
    static auto
    make_handler_list(HN&&... hn) ->
        handler_list_impl<sizeof...(HN)>
    {
        return handler_list_impl<sizeof...(HN)>(
            std::forward<HN>(hn)...);
    }

    template<class... HN>
    static auto
    make_except_list(HN&&... hn) ->
        handler_list_impl<sizeof...(HN)>
    {
        return handler_list_impl<sizeof...(HN)>(
            0, std::forward<HN>(hn)...);
    }

    void append(layer& e,
        http_proto::method verb,
        handler_list const& handlers)
    {
        add_impl(e, verb, handlers);
    }
};

//-----------------------------------------------

template<class Params>
class basic_router<Params>::
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
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler.
        // Error handlers and routers cannot be passed here.
        BOOST_CORE_STATIC_ASSERT(handler_check<1, H1, HN...>::value);
        owner_.add_impl(e_, core::string_view(), make_handler_list(
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
        http_proto::method verb,
        H1&& h1, HN&&... hn) ->
            fluent_route
    {
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler.
        // Error handlers and routers cannot be passed here.
        BOOST_CORE_STATIC_ASSERT(handler_check<1, H1, HN...>::value);
        owner_.add_impl(e_, verb, make_handler_list(
            std::forward<H1>(h1), std::forward<HN>(hn)...));
        return *this;
    }

    /** Add handlers for a method string.

        This registers regular handlers for the given HTTP method string
        on the current route, participating in dispatch as described in
        the @ref basic_router class documentation. This overload is
        intended for methods not represented by @ref http_proto::method.
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
        core::string_view verb,
        H1&& h1, HN&&... hn) ->
            fluent_route
    {
        // If you get a compile error on this line it means that
        // one or more of the provided types is not a valid handler.
        // Error handlers and routers cannot be passed here.
        BOOST_CORE_STATIC_ASSERT(handler_check<1, H1, HN...>::value);
        owner_.add_impl(e_, verb, make_handler_list(
            std::forward<H1>(h1), std::forward<HN>(hn)...));
        return *this;
    }

private:
    friend class basic_router;
    fluent_route(
        basic_router& owner,
        core::string_view pattern)
        : e_(owner.new_layer(pattern))
        , owner_(owner)
    {
    }

    layer& e_;
    basic_router& owner_;
};

} // http_proto
} // boost

#endif