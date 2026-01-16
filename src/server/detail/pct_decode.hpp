//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_DETAIL_PCT_DECODE_HPP
#define BOOST_HTTP_SERVER_DETAIL_PCT_DECODE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/url/pct_string_view.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include <string>

namespace boost {
namespace http {
namespace detail {

bool
ci_is_equal(
    core::string_view s0,
    core::string_view s1) noexcept;

// decode all percent escapes
std::string
pct_decode(
    urls::pct_string_view s);

// decode all percent escapes except slashes '/' and '\'
std::string
pct_decode_path(
    urls::pct_string_view s);

} // detail
} // http
} // boost

#endif
