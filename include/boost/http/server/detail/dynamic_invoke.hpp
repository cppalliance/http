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

// Polystore lookup key: also strips pointer
template<class T>
using lookup_key_t =
    std::remove_cv_t<
        std::remove_pointer_t<
            find_key_t<T>>>;

// True when parameter is a pointer (optional dependency)
template<class T>
constexpr bool is_optional_v =
    std::is_pointer_v<find_key_t<T>>;

// Resolve a polystore pointer to the handler's expected arg
template<class Arg, class T>
auto resolve_arg(T* p)
{
    if constexpr (is_optional_v<Arg>)
        return static_cast<find_key_t<Arg>>(p);
    else
        return static_cast<Arg>(*p);
}

//------------------------------------------------

template<class F, class... Args>
route_result
dynamic_invoke_impl(
    polystore& ps,
    F const& f,
    type_list<Args...> const&)
{
    static_assert(
        are_unique<lookup_key_t<Args>...>::value,
        "callable has duplicate parameter types");

    auto ptrs = std::make_tuple(
        ps.find<lookup_key_t<Args>>()...);

    return [&]<std::size_t... I>(
        std::index_sequence<I...>) -> route_result
    {
        if constexpr (!(is_optional_v<Args> && ...))
        {
            if(! (... && (is_optional_v<Args> ||
                std::get<I>(ptrs) != nullptr)))
                return route_next;
        }
        return f(resolve_arg<Args>(
            std::get<I>(ptrs))...);
    }(std::index_sequence_for<Args...>{});
}

/** Invoke a callable, resolving arguments from a polystore.

    Each parameter type of the callable is looked up in the
    polystore via @ref polystore::find. If all required
    parameters are found, the callable is invoked with the
    resolved arguments and its result is returned. If any
    required parameter is not found, @ref route_next is
    returned without invoking the callable.

    Parameters declared as pointer types (e.g. `A*`) are
    optional: `nullptr` is passed when the type is absent.
    Rvalue reference parameters (e.g. `A&&`) are supported
    and receive a moved reference to the stored object.

    Duplicate parameter types (after stripping cv-ref and
    pointer) produce a compile-time error.

    @param ps The polystore to resolve arguments from.
    @param f The callable to invoke.
    @return The result of the callable, or @ref route_next
    if any required parameter was not found.
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

/** Wraps a callable whose first parameter is `route_params&`
    and whose remaining parameters are resolved from
    @ref route_params::route_data at dispatch time.

    Produced by @ref dynamic_transform. Stored inside
    the router's handler table.
*/
template<class F>
struct dynamic_handler
{
    F f;

    // No extra parameters -- just forward to the callable
    template<class First>
    route_task
    invoke_impl(
        route_params& p,
        type_list<First> const&) const
    {
        static_assert(
            std::is_convertible_v<route_params&, First>,
            "first parameter must accept route_params&");
        using R = std::invoke_result_t<
            F const&, route_params&>;
        if constexpr (std::is_same_v<R, route_task>)
            return f(p);
        else
            return wrap_result(f(p));
    }

    static route_task
    make_route_next()
    {
        co_return route_next;
    }

    static route_task
    wrap_result(route_result r)
    {
        co_return r;
    }

    // Extra parameters resolved from route_data
    template<class First, class E1, class... Extra>
    route_task
    invoke_impl(
        route_params& p,
        type_list<First, E1, Extra...> const&) const
    {
        static_assert(
            std::is_convertible_v<route_params&, First>,
            "first parameter must accept route_params&");
        return invoke_extras(p, type_list<E1, Extra...>{});
    }

    template<class... Extras>
    route_task
    invoke_extras(
        route_params& p,
        type_list<Extras...> const&) const
    {
        static_assert(
            are_unique<
                lookup_key_t<Extras>...>::value,
            "callable has duplicate parameter types");

        auto ptrs = std::make_tuple(
            p.route_data.template find<
                lookup_key_t<Extras>>()...);

        return [this, &p, &ptrs]<std::size_t... I>(
            std::index_sequence<I...>) -> route_task
        {
            if constexpr (!(is_optional_v<Extras> && ...))
            {
                if(! (... && (is_optional_v<Extras> ||
                    std::get<I>(ptrs) != nullptr)))
                    return make_route_next();
            }

            using R = std::invoke_result_t<
                F const&, route_params&, Extras...>;
            if constexpr (std::is_same_v<R, route_task>)
                return f(p, resolve_arg<Extras>(
                    std::get<I>(ptrs))...);
            else
                return wrap_result(f(p,
                    resolve_arg<Extras>(
                        std::get<I>(ptrs))...));
        }(std::index_sequence_for<Extras...>{});
    }

    route_task
    operator()(route_params& p) const
    {
        return invoke_impl(p,
            typename call_traits<
                std::decay_t<F>>::arg_types{});
    }
};

/** A handler transform that resolves extra parameters from route_data.

    When used with @ref router::with_transform, handlers may
    declare a first parameter of type `route_params&` followed
    by additional parameters of arbitrary types. At dispatch time,
    each extra parameter type is looked up in
    @ref route_params::route_data via @ref polystore::find.
    If all required parameters are found the handler is invoked;
    otherwise @ref route_next is returned.

    Parameters declared as pointer types (e.g. `A*`) are
    optional: `nullptr` is passed when the type is absent.
    Rvalue reference parameters (e.g. `A&&`) are supported
    and receive a moved reference to the stored object.

    Duplicate extra parameter types (after stripping cv-ref
    and pointer) produce a compile-time error.

    @par Example
    @code
    router<route_params> base;
    auto r = base.with_transform( dynamic_transform{} );

    r.get( "/users", [](
        route_params& p,
        UserService& svc,
        Config const& cfg) -> route_result
    {
        // svc and cfg resolved from p.route_data
        return route_done;
    });
    @endcode
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
