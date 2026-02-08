//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_UPGRADE_RULE_HPP
#define BOOST_HTTP_RFC_UPGRADE_RULE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/rfc/list_rule.hpp>
#include <boost/core/detail/string_view.hpp>

namespace boost {
namespace http {

//------------------------------------------------

/** An Upgrade protocol
*/
struct upgrade_protocol
{
    /** The name of the protocol
    */
    core::string_view name;

    /** Optional protocol version

        An empty version indicates a
        version is not present.
    */
    core::string_view version;
};

//------------------------------------------------

namespace implementation_defined {
struct upgrade_protocol_rule_t
{
    using value_type = upgrade_protocol;

    BOOST_HTTP_DECL
    auto
    parse(
        char const*& it,
        char const* end) const noexcept ->
            system::result<value_type>;
};
} // implementation_defined

/** Rule to match Upgrade protocol

    @par Value Type
    @code
    using value_type = upgrade_protocol;
    @endcode

    @par Example
    @code
    @endcode

    @par BNF
    @code
    protocol         = protocol-name ["/" protocol-version]
    protocol-name    = token
    protocol-version = token
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-6.7"
        >6.7.  Upgrade (rfc7230)</a>

    @see
        @ref upgrade_protocol.
*/
BOOST_INLINE_CONSTEXPR implementation_defined::upgrade_protocol_rule_t upgrade_protocol_rule{};

//------------------------------------------------

/** Rule matching the Upgrade field value

    @par Value Type
    @code
    using value_type = grammar::range< upgrade_protocol >;
    @endcode

    @par Example
    @code
    @endcode

    @par BNF
    @code
    Upgrade          = 1#protocol

    protocol         = protocol-name ["/" protocol-version]
    protocol-name    = token
    protocol-version = token
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-6.7"
        >6.7.  Upgrade (rfc7230)</a>

    @see
        @ref upgrade_protocol.
*/
BOOST_INLINE_CONSTEXPR auto upgrade_rule = list_rule( upgrade_protocol_rule, 1 );

} // http
} // boost

#endif
