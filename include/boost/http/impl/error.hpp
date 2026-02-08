//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_IMPL_ERROR_HPP
#define BOOST_HTTP_IMPL_ERROR_HPP

#include <boost/core/detail/string_view.hpp>
#include <boost/system/error_category.hpp>
#include <boost/system/is_error_code_enum.hpp>
#include <boost/system/is_error_condition_enum.hpp>
#include <system_error>

namespace boost {

namespace system {

template<>
struct is_error_code_enum<
    ::boost::http::error>
{
    static bool const value = true;
};

template<>
struct is_error_condition_enum<
    ::boost::http::condition>
{
    static bool const value = true;
};

} // system
} // boost

namespace std {
template<>
struct is_error_code_enum<
    ::boost::http::error>
    : std::true_type {};

template<>
struct is_error_condition_enum<
    ::boost::http::condition>
    : std::true_type {};
} // std

namespace boost {

//-----------------------------------------------

namespace http {

namespace detail {

struct BOOST_HTTP_SYMBOL_VISIBLE
    error_cat_type
    : system::error_category
{
    BOOST_HTTP_DECL const char* name(
        ) const noexcept override;
    BOOST_HTTP_DECL std::string message(
        int) const override;
    BOOST_HTTP_DECL char const* message(
        int, char*, std::size_t
            ) const noexcept override;
    BOOST_SYSTEM_CONSTEXPR error_cat_type()
        : error_category(0x3663257e7585fbfd)
    {
    }
};

struct BOOST_HTTP_SYMBOL_VISIBLE
    condition_cat_type
    : system::error_category
{
    BOOST_HTTP_DECL const char* name(
        ) const noexcept override;
    BOOST_HTTP_DECL std::string message(
        int) const override;
    BOOST_HTTP_DECL char const* message(
        int, char*, std::size_t
            ) const noexcept override;
    BOOST_HTTP_DECL bool equivalent(
        system::error_code const&, int
            ) const noexcept override;
    BOOST_SYSTEM_CONSTEXPR condition_cat_type()
        : error_category(0xa36e10f16c666a7)
    {
    }
};

BOOST_HTTP_DECL extern
    error_cat_type error_cat;
BOOST_HTTP_DECL extern
    condition_cat_type condition_cat;

} // detail

inline
BOOST_SYSTEM_CONSTEXPR
system::error_code
make_error_code(
    error ev) noexcept
{
    return system::error_code{
        static_cast<std::underlying_type<
            error>::type>(ev),
        detail::error_cat};
}

inline
BOOST_SYSTEM_CONSTEXPR
system::error_condition
make_error_condition(
    condition c) noexcept
{
    return system::error_condition{
        static_cast<std::underlying_type<
            condition>::type>(c),
        detail::condition_cat};
}

} // http
} // boost

#endif
