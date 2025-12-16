//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_RFC_CONTENT_DISPOSITION_RULE_HPP
#define BOOST_HTTP_PROTO_RFC_CONTENT_DISPOSITION_RULE_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/rfc/parameter.hpp>
#include <boost/url/grammar/range_rule.hpp>

namespace boost {
namespace http_proto {

namespace implementation_defined {
struct content_disposition_rule_t
{
    struct value_type
    {
        core::string_view type;
        grammar::range<parameter> params;
    };

    BOOST_HTTP_PROTO_DECL
    system::result<value_type>
    parse(
        char const*& it,
        char const* end) const noexcept;
};
} // implementation_defined

/** Rule matching content-disposition

    @par Value Type
    @code
    struct value_type
    {
        core::string_view type;
        grammar::range< parameter > params;
    };
    @endcode

    @par Example
    @code
    @endcode

    @par BNF
    @code
    content-disposition = disposition-type *( OWS ";" OWS disposition-parm )

    disposition-type    = token
    disposition-parm    = token "=" ( token / quoted-string )
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc6266#section-4.1"
        >4.1.  Grammar (rfc6266)</a>
    @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-3.2.6"
        >3.2.6.  Field Value Components (rfc7230)</a>

    @see
        @ref quoted_token_view.
*/
BOOST_INLINE_CONSTEXPR implementation_defined::content_disposition_rule_t content_disposition_rule{};

} // http_proto
} // boost

#endif
