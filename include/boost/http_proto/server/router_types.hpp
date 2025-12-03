//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_SERVER_ROUTER_TYPES_HPP
#define BOOST_HTTP_PROTO_SERVER_ROUTER_TYPES_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/method.hpp>
#include <boost/http_proto/detail/except.hpp>
#include <boost/core/detail/string_view.hpp>
#include <boost/system/error_code.hpp>
#include <exception>
#include <string>
#include <type_traits>

namespace boost {
namespace http_proto {

/** The result type returned by a route handler.

    Route handlers use this type to report errors that prevent
    normal processing. A handler must never return a non-failing
    (i.e. `ec.failed() == false`) value. Returning a default-constructed
    `system::error_code` is disallowed; handlers that complete
    successfully must instead return a valid @ref route result.
*/
using route_result = system::error_code;

/** Route handler return values

    These values determine how the caller proceeds after invoking
    a route handler. Each enumerator represents a distinct control
    action—whether the request was handled, should continue to the
    next route, transfers ownership of the session, or signals that
    the connection should be closed.
*/
enum class route
{
    /** The handler requests that the connection be closed.

        No further requests will be processed. The caller should
        close the connection once the current response, if any,
        has been sent.
    */
    close = 1,

    /** The handler completed the request.

        The response has been fully transmitted, and no further
        handlers or routes will be invoked. The caller should continue
        by either reading the next request on a persistent connection
        or closing the session if it is not keep-alive.
    */
    complete,

    /** The handler detached from the session.

        Ownership of the session or stream has been transferred to
        the handler. The caller will not perform further I/O or manage
        the connection after this return value.
    */
    detach,

    /** The handler declined to process the request.

        The handler chose not to generate a response. The caller
        continues invoking the remaining handlers in the same route
        until one returns @ref send. If none do, the caller proceeds
        to evaluate the next matching route.

        This value is returned by @ref basic_router::dispatch if no
        handlers in any route handle the request.
    */
    next,

    /** The handler declined the current route.

        The handler wishes to skip any remaining handlers in the
        current route and move on to the next matching route. The
        caller stops invoking handlers in this route and resumes
        evaluation with the next candidate route.
    */
    next_route,

    /** The request was handled.

        The route handler processed the request and prepared
        the response serializer. The caller will send the response
        before reading the next request or closing the connection.
    */
    send
};

//------------------------------------------------

} // http_proto
namespace system {
template<>
struct is_error_code_enum<
    ::boost::http_proto::route>
{
    static bool const value = true;
};
} // system
namespace http_proto {

namespace detail {
struct BOOST_SYMBOL_VISIBLE route_cat_type
    : system::error_category
{
    BOOST_HTTP_PROTO_DECL const char* name() const noexcept override;
    BOOST_HTTP_PROTO_DECL std::string message(int) const override;
    BOOST_HTTP_PROTO_DECL char const* message(
        int, char*, std::size_t) const noexcept override;
    BOOST_SYSTEM_CONSTEXPR route_cat_type()
        : error_category(0x51c90d393754ecdf )
    {
    }
};
BOOST_HTTP_PROTO_DECL extern route_cat_type route_cat;
} // detail

inline
BOOST_SYSTEM_CONSTEXPR
system::error_code
make_error_code(route ev) noexcept
{
    return system::error_code{static_cast<
        std::underlying_type<route>::type>(ev),
        detail::route_cat};
}

/** Return true if `rv` is a route result.

    A @ref route_result can hold any error code,
    and this function returns `true` only if `rv`
    holds a value from the @ref route enumeration.
*/
inline bool is_route_result(
    route_result rv) noexcept
{
    return &rv.category() == &detail::route_cat;
}

//------------------------------------------------

class resumer;

/** Function to detach a route handler from its session

    This holds an reference to an implementation
    which detaches the handler from its session.
*/
class detacher
{
public:
    /** Base class of the implementation
    */
    struct BOOST_SYMBOL_VISIBLE
        owner
    {
        BOOST_HTTP_PROTO_DECL virtual resumer do_detach();
        virtual void do_resume(route_result const&) = 0;
    };

    detacher() = default;
    detacher(detacher const&) = default;
    detacher& operator=(detacher const&) = default;

    explicit
    detacher(
        owner& who) noexcept
        : p_(&who)
    {
    }

    /** Detach and invoke the given function

        The function will be invoked with this equivalent signature:
        @code
        void( resumer );
        @endcode

        @return A @ref route_result equal to @ref route::detach
    */
    template<class F>
    route_result    
    operator()(F&& f);

private:
    friend resumer;
    // Clang doesn't consider uninstantiated templates
    // when checking for unused private fields.
    owner* p_
    #if defined(__clang__)
        __attribute__((unused))
    #endif
        = nullptr; 
};

//------------------------------------------------

/** Function to resume a route handler's session

    This holds a reference to an implementation
    which resumes the handler's session. The resume
    function is returned by calling @ref detach.
*/
class resumer
{
public:
    /** Constructor

        Default constructed resume functions will
        be empty. An exception is thrown when
        attempting to invoke an empty object.
    */
    resumer() = default;

    /** Constructor

        Copies of resume functions behave the same
        as the original
    */
    resumer(resumer const&) = default;

    /** Assignment

        Copies of resume functions behave the same
        as the original
    */
    resumer& operator=(resumer const&) = default;

    /** Constructor
    */
    explicit
    resumer(
        detacher::owner& who) noexcept
        : p_(&who)
    {
    }

    /** Resume the session

        A session is resumed as if the detached
        handler returned the route result in @p rv.

        @param ec The error code to resume with.

        @throw std::invalid_argument If the object is empty.
    */
    void operator()(
        route_result const& rv) const
    {
        if(! p_)
            detail::throw_invalid_argument();
        p_->do_resume(rv);
    }

private:
    detacher::owner* p_
    #if defined(__clang__)
        __attribute__((unused))
    #endif
        = nullptr; 
};

template<class F>
auto    
detacher::
operator()(F&& f) ->
    route_result
{
    if(! p_)
        detail::throw_logic_error();
    std::forward<F>(f)(p_->do_detach());
    return route::detach;
}

//------------------------------------------------

namespace detail {
class any_router;
} // detail
template<class>
class basic_router;

/** Base class for request objects

    This is a required public base for any `Request`
    type used with @ref basic_router.
*/
class route_params_base
{
public:
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

private:
    friend class /*detail::*/any_router;
    template<class>
    friend class basic_router;
    struct match_result;
    route_params_base& operator=(
        route_params_base const&) = delete;

    std::string verb_str_;
    std::string decoded_path_;
    system::error_code ec_;
    std::exception_ptr ep_;
    std::size_t pos_ = 0;
    std::size_t resume_ = 0;
    http_proto::method verb_ =
        http_proto::method::unknown;
    bool addedSlash_ = false;
    bool case_sensitive = false;
    bool strict = false;
};

} // http_proto
} // boost

#endif
