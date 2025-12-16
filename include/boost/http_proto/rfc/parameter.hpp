//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_RFC_PARAMETER_HPP
#define BOOST_HTTP_PROTO_RFC_PARAMETER_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/rfc/quoted_token_view.hpp>
#include <boost/system/result.hpp>
#include <boost/url/grammar/range_rule.hpp>

namespace boost {
namespace http_proto {

/** An HTTP header parameter

    @par BNF
    @code
    parameter   = token "=" ( token / quoted-string )
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc7231#section-3.1.1.1"
        >3.1.1.1.  Media Type (rfc7231)</a>
*/
struct parameter
{
    core::string_view name;
    quoted_token_view value;
};

//------------------------------------------------

namespace implementation_defined {
struct parameter_rule_t
{
    using value_type = parameter;

    BOOST_HTTP_PROTO_DECL
    auto
    parse(
        char const*&,
        char const*) const noexcept ->
            system::result<value_type>;
};

struct parameters_rule_t
{
    using value_type = grammar::range<parameter>;

    BOOST_HTTP_PROTO_DECL
    auto
    parse(
        char const*&,
        char const*) const noexcept ->
            system::result<value_type>;
};
} // implementation_defined

/** Rule matching parameter

    @par Value Type
    @code
    using value_type = parameter;
    @endcode

    @par Example
    @code
    @endcode

    @par BNF
    @code
    parameter   = token "=" ( token / quoted-string )
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc7231#section-3.1.1.1"
        >3.1.1.1.  Media Type (rfc7231)</a>

    @see
        @ref parameter.
*/
BOOST_INLINE_CONSTEXPR implementation_defined::parameter_rule_t parameter_rule{};

//------------------------------------------------

/** Rule matching parameters

    @par Value Type
    @code
    using value_type = grammar::range< parameter >;
    @endcode

    @par Example
    @code
    @endcode

    @par BNF
    @code
    parameters  = *( OWS ";" OWS parameter )
    parameter   = token "=" ( token / quoted-string )
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc7231#section-3.1.1.1"
        >3.1.1.1.  Media Type (rfc7231)</a>

    @see
        @ref parameter,
        @ref parameter_rule.
*/
BOOST_INLINE_CONSTEXPR implementation_defined::parameters_rule_t parameters_rule{};

} // http_proto
} // boost

#endif
