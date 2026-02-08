//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_QUOTED_TOKEN_RULE_HPP
#define BOOST_HTTP_RFC_QUOTED_TOKEN_RULE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/rfc/quoted_token_view.hpp>
#include <boost/system/result.hpp>
#include <boost/assert.hpp>

namespace boost {
namespace http {

namespace implementation_defined {
struct quoted_token_rule_t
{
    using value_type = quoted_token_view;

    BOOST_HTTP_DECL
    auto
    parse(
        char const*& it,
        char const* end) const noexcept ->
            system::result<value_type>;
};
} // implementation_defined

/** Rule matching quoted-token

    @par Value Type
    @code
    using value_type = quoted_token_view;
    @endcode

    @par Example
    @code
    @endcode

    @par BNF
    @code
    quoted-token    = token / quoted-string

    token           = 1*tchar
    quoted-string   = DQUOTE *( qdtext / quoted-pair ) DQUOTE
    qdtext          = HTAB / SP / 0x21 / 0x23-0x5B / 0x5D-0x7E / obs-text
    obs-text        = 0x80-0xFF
    quoted-pair     = "\"" ( HTAB / SP / VCHAR / obs-text )
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-3.2.6"
        >3.2.6.  Field Value Components (rfc7230)</a>

    @see
        @ref quoted_token_view.
*/
BOOST_INLINE_CONSTEXPR implementation_defined::quoted_token_rule_t quoted_token_rule{};

} // http
} // boost

#endif
