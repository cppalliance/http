//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_RFC_MEDIA_TYPE_HPP
#define BOOST_HTTP_PROTO_RFC_MEDIA_TYPE_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/rfc/parameter.hpp>
#include <boost/url/grammar/range_rule.hpp>

namespace boost {
namespace http_proto {

/** A mime-type
*/
struct mime_type
{
    /** The type
    */
    core::string_view type;

    /** The subtype
    */
    core::string_view subtype;
};

//------------------------------------------------

/** A media-type
*/
struct media_type
{
    /** The mime type
    */
    mime_type mime;

    /** Parameters
    */
    grammar::range<
        parameter> params;
};

//------------------------------------------------

namespace implementation_defined {
struct media_type_rule_t
{
    using value_type = media_type;

    BOOST_HTTP_PROTO_DECL
    auto
    parse(
        char const*& it,
        char const* end) const noexcept ->
            system::result<value_type>;
};
} // implementation_defined

/** Rule matching media-type

    @par BNF
    @code
    media-type  = type "/" subtype *( OWS ";" OWS parameter )
    parameter   = token "=" ( token / quoted-string )
    subtype     = token
    type        = token
    @endcode

    @par Specification
    @li <a href="https://www.rfc-editor.org/rfc/rfc7231#section-3.1.1.1"
        >3.1.1.1.  Media Type (rfc7231)</a>

    @see
        @ref media_type.
*/
BOOST_INLINE_CONSTEXPR implementation_defined::media_type_rule_t media_type_rule{};

} // http_proto
} // boost

#endif
