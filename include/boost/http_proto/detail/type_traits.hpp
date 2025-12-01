//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_DETAIL_TYPE_TRAITS_HPP
#define BOOST_HTTP_PROTO_DETAIL_TYPE_TRAITS_HPP

#include <functional>
#include <type_traits>

namespace boost {
namespace http_proto {
namespace detail {

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

template<bool...> struct bool_pack {};
template<bool... Bs>
struct all_true : std::is_same<bool_pack<
    true, Bs...>, bool_pack<Bs..., true>> {};

} // detail
} // http_proto
} // boost

#endif
