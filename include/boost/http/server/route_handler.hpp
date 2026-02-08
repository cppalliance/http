//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ROUTE_HANDLER_HPP
#define BOOST_HTTP_SERVER_ROUTE_HANDLER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/method.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/http/datastore.hpp>
#include <boost/http/request.hpp>
#include <boost/http/response.hpp>
#include <boost/core/detail/string_view.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/io_task.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/write.hpp>
#include <boost/capy/io/any_buffer_source.hpp>
#include <boost/capy/io/any_buffer_sink.hpp>
#include <boost/url/url_view.hpp>
#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>
#include <concepts>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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

template<class, class> class router;

namespace detail {

struct route_params_access;
class router_base;

struct route_params_base_privates
{
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
    char kind_ = 0;
};

} // detail

//------------------------------------------------

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
class BOOST_HTTP_SYMBOL_VISIBLE
    route_params
{
    detail::route_params_base_privates priv_;

public:
    struct match_result;

    /** Return true if the request method matches `m`
    */
    bool is_method(
        http::method m) const noexcept
    {
        return priv_.verb_ == m;
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

    /** Captured route parameters

        Contains name-value pairs extracted from the path
        by matching :param and *wildcard tokens.
    */
    std::vector<std::pair<std::string, std::string>> params;

    /// The complete request target
    urls::url_view url;

    /// The HTTP request
    http::request req;

    /// The HTTP response
    http::response res;

    /// Provides access to the request body
    capy::any_buffer_source req_body;

    /// Provides access to the response body
    capy::any_buffer_sink res_body;

    /// Arbitrary per-route data
    http::datastore route_data;

    /// Arbitrary per-session data
    http::datastore session_data;

    BOOST_HTTP_DECL ~route_params();
    BOOST_HTTP_DECL void reset();
    BOOST_HTTP_DECL route_params& status(http::status code);

    /** Send the response with an optional body.
    */
    BOOST_HTTP_DECL capy::io_task<> send(std::string_view body = {});

private:
    template<class, class>
    friend class router;
    friend class detail::router_base;
    friend struct detail::route_params_access;

    route_params& operator=(
        route_params const&) = delete;
};

struct route_params::
    match_result
{
    std::vector<std::pair<std::string, std::string>> params_;

    void adjust_path(
        route_params& p,
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
            p.path = { p.priv_.decoded_path_.data() +
                p.priv_.decoded_path_.size() - 1, 1};
            BOOST_ASSERT(p.path == "/");
        }
    }

    void restore_path(
        route_params& p)
    {
        if( n_ > 0 &&
            p.priv_.addedSlash_ &&
            p.path.data() ==
                p.priv_.decoded_path_.data() +
                p.priv_.decoded_path_.size() - 1)
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

//------------------------------------------------

namespace detail {

template<class H, class... Args>
concept returns_route_task = std::same_as<
    std::invoke_result_t<H, Args...>, route_task>;

struct route_params_access
{
    route_params& rp;

    route_params_base_privates& operator*() const noexcept
    {
        return rp.priv_;
    }

    route_params_base_privates* operator->() const noexcept
    {
        return &rp.priv_;
    }
};

} // detail

} // http
} // boost

#endif
