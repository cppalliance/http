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
#include <boost/system/error_code.hpp>
#include <exception>
#include <string>
#include <type_traits>

namespace boost {
namespace http {

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
    action�whether the request was handled, should continue to the
    next route, transfers ownership of the session, or signals that
    the connection should be closed.
*/
enum class route
{
    /** The handler declined to process the request.

        The handler chose not to generate a response. The caller
        continues invoking the remaining handlers in the same route
        until one returns @ref send. If none do, the caller proceeds
        to evaluate the next matching route.

        This value is returned by @ref router::dispatch if no
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

    /** The handler wants the connection closed.

        The handler has determined that the connection should be
        terminated. The caller stops invoking any remaining handlers
        and closes the connection without sending a response.
    */
    close
};

//------------------------------------------------

} // http
namespace system {
template<>
struct is_error_code_enum<
    ::boost::http::route>
{
    static bool const value = true;
};
} // system
namespace http {

namespace detail {
struct BOOST_HTTP_SYMBOL_VISIBLE route_cat_type
    : system::error_category
{
    BOOST_HTTP_DECL const char* name() const noexcept override;
    BOOST_HTTP_DECL std::string message(int) const override;
    BOOST_HTTP_DECL char const* message(
        int, char*, std::size_t) const noexcept override;
    BOOST_SYSTEM_CONSTEXPR route_cat_type()
        : error_category(0x51c90d393754ecdf )
    {
    }
};
BOOST_HTTP_DECL extern route_cat_type route_cat;
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
