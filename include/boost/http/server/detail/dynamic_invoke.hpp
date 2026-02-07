//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_DETAIL_DYNAMIC_INVOKE_HPP
#define BOOST_HTTP_SERVER_DETAIL_DYNAMIC_INVOKE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/core/polystore.hpp>
#include <boost/http/server/route_handler.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

namespace boost {
namespace http {
namespace detail {

//------------------------------------------------

template<class... Ts>
struct are_unique : std::true_type {};

template<class T, class... Ts>
struct are_unique<T, Ts...>
    : std::bool_constant<
        (!std::is_same_v<T, Ts> && ...) &&
        are_unique<Ts...>::value> {};

//------------------------------------------------

template<class T>
using find_key_t =
    std::remove_cv_t<
        std::remove_reference_t<T>>;

//------------------------------------------------

template<class F, class... Args>
route_result
dynamic_invoke_impl(
    polystore& ps,
    F const& f,
    type_list<Args...> const&)
{
    static_assert(
        are_unique<find_key_t<Args>...>::value,
        "callable has duplicate parameter types");

    auto ptrs = std::make_tuple(
        ps.find<find_key_t<Args>>()...);

    bool all_found = std::apply(
        [](auto*... p)
        {
            return (... && (p != nullptr));
        }, ptrs);

    if(! all_found)
        return route_next;

    return std::apply(
        [&](auto*... p) -> route_result
        {
            return f(*p...);
        }, ptrs);
}

/** Invoke a callable, resolving arguments from a polystore.

    Each parameter type of the callable is looked up in the
    polystore via @ref polystore::find. If all parameters are
    found, the callable is invoked with the resolved arguments
    and its result is returned. If any parameter is not found,
    @ref route_next is returned without invoking the callable.

    Duplicate parameter types (after stripping cv-ref) produce
    a compile-time error.

    @param ps The polystore to resolve arguments from.
    @param f The callable to invoke.
    @return The result of the callable, or @ref route_next
    if any parameter was not found.
*/
template<class F>
route_result
dynamic_invoke(
    polystore& ps,
    F const& f)
{
    return dynamic_invoke_impl(
        ps, f,
        typename call_traits<
            std::decay_t<F>>::arg_types{});
}

//------------------------------------------------

template<class F>
struct dynamic_handler
{
    F f;

    route_task
    operator()(route_params& p) const
    {
        co_return dynamic_invoke(
            p.route_data, f);
    }
};

/** A handler transform that resolves parameters from route_data.

    When used with @ref router::with_transform, handlers may
    declare parameters of arbitrary types. At dispatch time,
    each parameter type is looked up in @ref route_params::route_data.
    If all parameters are found the handler is invoked; otherwise
    @ref route_next is returned.
*/
struct dynamic_transform
{
    template<class F>
    auto
    operator()(F f) const ->
        dynamic_handler<std::decay_t<F>>
    {
        return { std::move(f) };
    }
};

} // detail
} // http
} // boost

#endif
