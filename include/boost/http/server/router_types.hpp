//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ROUTER_TYPES_HPP
#define BOOST_HTTP_SERVER_ROUTER_TYPES_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/method.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/core/detail/string_view.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>
#include <exception>
#include <string>
#include <type_traits>

namespace boost {
namespace http {

/** Directive values for route handler results.

    These values indicate how the router should proceed
    after a handler completes. Handlers return one of
    the predefined constants (@ref route_done, @ref route_next,
    @ref route_next_route, @ref route_close) or an error code.

    @see route_result, route_task
*/
enum class route_what
{
    /// Handler completed successfully, response was sent
    done,

    /// Handler declined, try next handler in the route
    next,

    /// Handler declined, skip to next matching route
    next_route,

    /// Handler requests connection closure
    close,

    /// Handler encountered an error
    error
};

//------------------------------------------------

/** The result type returned by route handlers.

    This class represents the outcome of a route handler.
    Handlers return this type to indicate how the router
    should proceed. Construct from a directive constant
    or an error code:

    @code
    route_task my_handler(route_params& p)
    {
        if(! authorized(p))
            co_return route_next;        // try next handler

        if(auto ec = process(p); ec)
            co_return ec;                // return error

        co_return route_done;            // success
    }
    @endcode

    @par Checking Results

    Use @ref what() to determine the directive, and
    @ref error() to retrieve any error code:

    @code
    route_result rv = co_await handler(p);
    if(rv.what() == route_what::error)
        handle_error(rv.error());
    @endcode

    @see route_task, route_what, route_done, route_next
*/
class BOOST_HTTP_DECL
    route_result
{
    system::error_code ec_;

    template<route_what T>
    struct what_t {};

    route_result(system::error_code ec);
    void set(route_what w);

public:
    route_result() = default;

    /** Construct from a directive constant.

        This constructor allows implicit conversion from
        the predefined constants (@ref route_done, @ref route_next,
        @ref route_next_route, @ref route_close).

        @code
        route_task handler(route_params& p)
        {
            co_return route_done;  // implicitly converts
        }
        @endcode
    */
    template<route_what W>
    route_result(what_t<W>)
    {
        static_assert(W != route_what::error);
        set(W);
    }

    /** Return the directive for this result.

        Call this to determine how the router should proceed:

        @code
        route_result rv = co_await handler(p);
        switch(rv.what())
        {
        case route_what::done:
            // response sent, done with request
            break;
        case route_what::next:
            // try next handler
            break;
        case route_what::error:
            log_error(rv.error());
            break;
        }
        @endcode

        @return The directive value.
    */
    auto
    what() const noexcept ->
        route_what;

    /** Return the error code, if any.

        If @ref what() returns `route_what::error`, this
        returns the underlying error code. Otherwise returns
        a default-constructed (non-failing) error code.

        @return The error code, or a non-failing code.
    */
    auto
    error() const noexcept ->
        system::error_code;

    /** Return true if the result indicates an error.

        @return `true` if @ref what() equals `route_what::error`.
    */
    bool failed() const noexcept
    {
        return what() == route_what::error;
    }

    static constexpr route_result::what_t<route_what::done> route_done{};
    static constexpr route_result::what_t<route_what::next> route_next{};
    static constexpr route_result::what_t<route_what::next_route> route_next_route{};
    static constexpr route_result::what_t<route_what::close> route_close{};
    friend route_result route_error(system::error_code ec) noexcept;

    template<class E>
    friend auto route_error(E e) noexcept ->
        std::enable_if_t<
            system::is_error_code_enum<E>::value,
            route_result>;
};

//------------------------------------------------

/** Handler completed successfully.

    Return this from a handler to indicate the response
    was sent and the request is complete:

    @code
    route_task handler(route_params& p)
    {
        p.res.set(field::content_type, "text/plain");
        co_await p.send("Hello, World!");
        co_return route_done;
    }
    @endcode
*/
inline constexpr decltype(auto) route_done = route_result::route_done;

/** Handler declined, try next handler.

    Return this from a handler to decline processing
    and allow the next handler in the route to try:

    @code
    route_task auth_handler(route_params& p)
    {
        if(! p.req.exists(field::authorization))
            co_return route_next;  // let another handler try

        // process authenticated request...
        co_return route_done;
    }
    @endcode
*/
inline constexpr decltype(auto) route_next = route_result::route_next;

/** Handler declined, skip to next route.

    Return this from a handler to skip all remaining
    handlers in the current route and proceed to the
    next matching route:

    @code
    route_task version_check(route_params& p)
    {
        if(p.req.version() < 11)
            co_return route_next_route;  // skip this route

        co_return route_next;  // continue with this route
    }
    @endcode
*/
inline constexpr decltype(auto) route_next_route = route_result::route_next_route;

/** Handler requests connection closure.

    Return this from a handler to immediately close
    the connection without sending a response:

    @code
    route_task ban_check(route_params& p)
    {
        if(is_banned(p.req.remote_address()))
            co_return route_close;  // drop connection

        co_return route_next;
    }
    @endcode
*/
inline constexpr decltype(auto) route_close = route_result::route_close;

/** Construct from an error code.

    Use this constructor to return an error from a handler.
    The error code must represent a failure condition.

    @param ec The error code to return.

    @throw std::invalid_argument if `!ec` (non-failing code).
*/
inline route_result route_error(system::error_code ec) noexcept
{
    return route_result(ec);
}

/** Construct from an error enum.

    Use this overload to return an error from a handler
    using any type satisfying `is_error_code_enum`.

    @param e The error enum value to return.
*/
template<class E>
auto route_error(E e) noexcept ->
    std::enable_if_t<
        system::is_error_code_enum<E>::value,
        route_result>
{
    return route_result(make_error_code(e));
}

//------------------------------------------------

/** Convenience alias for route handler return type.

    Route handlers are coroutines that return a @ref route_result
    indicating how the router should proceed. This alias simplifies
    handler declarations:

    @code
    route_task my_handler(route_params& p)
    {
        // process request...
        co_return route_done;
    }

    route_task auth_middleware(route_params& p)
    {
        if(! check_token(p))
        {
            p.res.set_status(status::unauthorized);
            co_await p.send();
            co_return route_done;
        }
        co_return route_next;  // continue to next handler
    }
    @endcode

    @see route_result, route_params
*/
using route_task = capy::task<route_result>;

//------------------------------------------------

namespace detail {
class router_base;
} // detail
template<class> class basic_router;

struct route_params_base_privates
{
    struct match_result;

    std::string verb_str_;
    std::string decoded_path_;
    system::error_code ec_;
    std::exception_ptr ep_;
    std::size_t pos_ = 0;
    std::size_t resume_ = 0;
    http::method verb_ =
        http::method::unknown;
    bool addedSlash_ = false;
    bool case_sensitive = false;
    bool strict = false;
    char kind_ = 0;  // dispatch mode, initialized by flat_router::dispatch()
};

/** Base class for request objects

    This is a required public base for any `Request`
    type used with @ref router.
*/
class route_params_base : public route_params_base_privates
{
public:
    /** Return true if the request method matches `m`
    */
    bool is_method(
        http::method m) const noexcept
    {
        return verb_ == m;
    }

    /** Return true if the request method matches `s`
    */
    BOOST_HTTP_DECL
    bool is_method(
        core::string_view s) const noexcept;

    /** The mount path of the current router

        This is the portion of the request path
        which was matched to select the handler.
        The remaining portion is available in
        @ref path.
    */
    core::string_view base_path;

    /** The current pathname, relative to the base path
    */
    core::string_view path;

   struct match_result;

private:
    template<class>
    friend class basic_router;
    friend struct route_params_access;

    route_params_base& operator=(
        route_params_base const&) = delete;
};

struct route_params_base::
    match_result
{
    void adjust_path(
        route_params_base& p,
        std::size_t n)
    {
        n_ = n;
        if(n_ == 0)
            return;
        p.base_path = {
            p.base_path.data(),
            p.base_path.size() + n_ };
        if(n_ < p.path.size())
        {
            p.path.remove_prefix(n_);
        }
        else
        {
            // append a soft slash
            p.path = { p.decoded_path_.data() +
                p.decoded_path_.size() - 1, 1};
            BOOST_ASSERT(p.path == "/");
        }
    }

    void restore_path(
        route_params_base& p)
    {
        if( n_ > 0 &&
            p.addedSlash_ &&
            p.path.data() ==
                p.decoded_path_.data() +
                p.decoded_path_.size() - 1)
        {
            // remove soft slash
            p.path = {
                p.base_path.data() +
                p.base_path.size(), 0 };
        }
        p.base_path.remove_suffix(n_);
        p.path = {
            p.path.data() - n_,
            p.path.size() + n_ };
    }

private:
    std::size_t n_ = 0; // chars moved from path to base_path
};


namespace detail {

struct route_params_access
{
    route_params_base& rp;

    route_params_base_privates* operator->() const noexcept
    {
        return &rp;
    }
};

} // detail

} // http
} // boost

#endif
