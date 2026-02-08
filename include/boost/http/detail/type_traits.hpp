//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_TYPE_TRAITS_HPP
#define BOOST_HTTP_DETAIL_TYPE_TRAITS_HPP

#if __cplusplus >= 202002L
#include <utility>
#else
#include <functional>
#endif
#include <type_traits>

namespace boost {
namespace http {
namespace detail {

template<class...> struct make_void { typedef void type; };
template<class... Ts> using void_t = typename make_void<Ts...>::type;

template<class T>
struct remove_cvref
{
    using type =
        typename std::remove_cv<
        typename std::remove_reference<T>::type>::type;
};

template<class T>
using remove_cvref_t = typename remove_cvref<T>::type;

template<class T>
struct is_reference_wrapper_impl
    : std::false_type
{
};

template<class T>
struct is_reference_wrapper_impl<
    std::reference_wrapper<T>>
    : std::true_type
{
};

#if !defined(BOOST_NO_CV_SPECIALIZATIONS)

template<class T>
struct is_reference_wrapper_impl<
    std::reference_wrapper<T> const>
    : std::true_type
{
};

template<class T>
struct is_reference_wrapper_impl<
    std::reference_wrapper<T> volatile>
    : std::true_type
{
};

template<class T>
struct is_reference_wrapper_impl<
    std::reference_wrapper<T> const volatile>
    : std::true_type
{
};

#endif

template<class T>
using is_reference_wrapper =
    is_reference_wrapper_impl<remove_cvref_t<T>>;

template<class Base, class Derived>
using derived_from = std::integral_constant<bool,
    std::is_base_of<Base, Derived>::value &&
    std::is_convertible<
        Derived const volatile*,
        Base const volatile*>::value>;

} // detail
} // http
} // boost

#endif
