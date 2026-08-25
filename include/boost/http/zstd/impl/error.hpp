//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZSTD_IMPL_ERROR_HPP
#define BOOST_HTTP_ZSTD_IMPL_ERROR_HPP

#include <boost/http/detail/config.hpp>

#include <boost/system/error_category.hpp>
#include <boost/system/is_error_code_enum.hpp>
#include <system_error>

namespace boost {

namespace system {
template<>
struct is_error_code_enum<
    ::boost::http::zstd::error>
{
    static bool const value = true;
};
} // system
} // boost

namespace std {
template<>
struct is_error_code_enum<
    ::boost::http::zstd::error>
    : std::true_type {};
} // std

namespace boost {
namespace http {
namespace zstd {

namespace detail {

struct BOOST_SYMBOL_VISIBLE
    error_cat_type
    : system::error_category
{
    BOOST_HTTP_DECL const char* name(
        ) const noexcept override;
    BOOST_HTTP_DECL bool failed(
        int) const noexcept override;
    BOOST_HTTP_DECL std::string message(
        int) const override;
    BOOST_HTTP_DECL char const* message(
        int, char*, std::size_t
            ) const noexcept override;
    BOOST_SYSTEM_CONSTEXPR error_cat_type()
        : error_category(0x9971e0803a6de4e7)
    {
    }
};

BOOST_HTTP_DECL extern
    error_cat_type error_cat;

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

} // zstd
} // http
} // boost

#endif
