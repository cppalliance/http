//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_TOKEN_RULE_HPP
#define BOOST_HTTP_RFC_TOKEN_RULE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/url/grammar/lut_chars.hpp>
#include <boost/url/grammar/token_rule.hpp>

namespace boost {
namespace http {

/** The set of token characters

    @par BNF
    @code
    tchar       = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                / DIGIT / ALPHA
                ; any VCHAR, except delimiters

    VCHAR       =  0x21-0x7E
    @endcode

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc7230#section-3.2.3"
        >3.2.3. Whitespace (rfc7230)</a>
    @li <a href="https://datatracker.ietf.org/doc/html/rfc5234#appendix-B.1"
        >B.1. Core Rules (rfc5234)</a>
*/
BOOST_INLINE_CONSTEXPR grammar::lut_chars tchars =
    "!#$%&'*+-.^_`|~"
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    ;

/** Match a token

    @par Value Type
    @code
    using value_type = string_view;
    @endcode

    @par Example
    @code
    @endcode

    @par BNF
    @code
    token           = 1*tchar
    @endcode
*/
BOOST_INLINE_CONSTEXPR auto token_rule = grammar::token_rule( tchars );

} // http
} // boost

#endif
