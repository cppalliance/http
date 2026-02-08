//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_HEADER_LIMITS_HPP
#define BOOST_HTTP_HEADER_LIMITS_HPP

#include <boost/http/detail/config.hpp>
#include <cstddef>

namespace boost {
namespace http {

/** Configurable limits for HTTP headers.

    Objects of this type are used to configure
    upper limits for HTTP headers.
*/
struct header_limits
{
    /** Largest allowed size for complete headers.

        This determines an upper bound on the
        allowed size of the start-line plus all
        of the individual fields in the headers.
        This counts all delimiters including
        trailing CRLFs.

        @par ABNF
        @code
        HTTP-headers   = start-line
                         *( field-line CRLF )
                         CRLF
        field-line     = field-name ":" OWS field-value OWS
        @endcode

        @see
            @li <a href="https://datatracker.ietf.org/doc/html/rfc9112#section-2.1"
                >2.1.  Message Format (rfc9112)</a>
            @li <a href="https://datatracker.ietf.org/doc/html/rfc9112#section-5"
                >5.  Field Syntax (rfc9112)</a>@see
            @li <a href="https://stackoverflow.com/questions/686217/maximum-on-http-header-values"
                >Maximum on HTTP header values (Stackoverflow)</a>
    */
    std::size_t max_size = 8 * 1024;

    /** Largest allowed size for the start-line.

        This determines an upper bound on the
        allowed size for the request-line of
        an HTTP request or the status-line of
        an HTTP response.

        @par ABNF
        @code
        start-line     = request-line / status-line
        request-line   = method SP request-target SP HTTP-version CRLF
        status-line    = HTTP-version SP status-code SP reason-phrase CRLF
        @endcode

        @see
            @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-3.1"
                >3.1.  Start Line (rfc7230)</a>
            @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-3.1.1"
                >3.1.1.  Request Line (rfc7230)</a>
            @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-3.1.2"
                >3.1.2.  Status Line (rfc7230)</a>
    */
    std::size_t max_start_line = 4096;

    /** Largest size for one field.

        This determines an upper bound on the
        allowed size for any single header in
        an HTTP message. This counts the field
        name, field value, and delimiters
        including a trailing CRLF.

        @par ABNF
        @code
        header-field   = field-name ":" OWS field-value OWS
        field-value    = *( field-content / obs-fold )
        field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
        field-vchar    = VCHAR / obs-text

        obs-fold       = CRLF 1*( SP / HTAB )
                       ; obsolete line folding
                       ; see Section 3.2.4
        @endcode

        @see
            @li <a href="https://www.rfc-editor.org/rfc/rfc7230#section-3.2"
                >3.2.  Header Fields (rfc7230)</a>
    */
    std::size_t max_field = 4096;

    /** Largest allowed number of fields.

        This determines an upper bound on the
        largest number of individual header
        fields that may appear in an HTTP message.
        Depending on the other  limits, the actual
        maximum number of fields might be less
        than this value.
    */
    std::size_t max_fields = 100;

    /** Return the storage space required for these settings.

        This function returns the largest
        number of contiguous bytes of storage
        that would be needed at these settings.
    */
    BOOST_HTTP_DECL
    std::size_t
    valid_space_needed() const;
};

} // http
} // boost

#endif
