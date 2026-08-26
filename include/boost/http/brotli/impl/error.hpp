//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_BROTLI_IMPL_ERROR_HPP
#define BOOST_HTTP_BROTLI_IMPL_ERROR_HPP

#include <boost/http/detail/config.hpp>

#include <system_error>

namespace std {
template<>
struct is_error_code_enum<
    ::boost::http::brotli::error>
    : std::true_type {};
} // std

namespace boost {
namespace http {
namespace brotli {

namespace detail {

struct BOOST_SYMBOL_VISIBLE
    error_cat_type
    : std::error_category
{
    BOOST_HTTP_DECL const char* name(
        ) const noexcept override;
    BOOST_HTTP_DECL std::string message(
        int) const override;
    constexpr error_cat_type() noexcept = default;
};

BOOST_HTTP_DECL extern
    error_cat_type error_cat;

} // detail

inline
std::error_code
make_error_code(
    error ev) noexcept
{
    return std::error_code{
        static_cast<std::underlying_type<
            error>::type>(ev),
        detail::error_cat};
}

} // brotli
} // http
} // boost

#endif
