//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_ACCEPTS_HPP
#define BOOST_HTTP_SERVER_ACCEPTS_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/fields_base.hpp>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace boost {
namespace http {

/** Content negotiation based on request headers.

    This class examines the Accept, Accept-Encoding,
    Accept-Charset, and Accept-Language request headers
    to determine which content types, encodings, charsets,
    and languages the client prefers.

    @par Example
    @code
    accepts ac( req );

    // Get the preferred type from offered list
    auto t = ac.type({ "html", "json", "text/plain" });

    // Get all accepted media types sorted by preference
    auto all = ac.types();
    @endcode

    @see mime_types
*/
class BOOST_HTTP_DECL accepts
{
    fields_base const& fields_;

public:
    /** Construct from message fields.

        @param fields The message fields to negotiate against.
    */
    explicit
    accepts( fields_base const& fields ) noexcept;

    /** Return the preferred media type from the offered list.

        Each value may be a MIME type such as "application/json",
        or a file extension such as "json". The best match is
        returned based on the request's Accept header.

        If the Accept header is absent, the first offered type
        is returned. Returns an empty string view if no offered
        type is acceptable.

        @par Example
        @code
        accepts ac( req );

        // Accept: application/json
        ac.type({ "json" });  // => "json"
        ac.type({ "html" });  // => "" (empty)

        // Accept: application/json, text/html;q=0.5
        ac.type({ "html", "json" }); // => "json"
        @endcode

        @param offered The types to negotiate from (MIME types
        or file extensions).

        @return The best matching type from the offered
        list, or an empty string view if none match.
    */
    std::string_view
    type(
        std::initializer_list<
            std::string_view> offered ) const;

    /** Return all accepted media types sorted by preference.

        Returns the media types from the Accept header,
        ordered by quality value.

        @return A vector of accepted media types.
    */
    std::vector<std::string_view>
    types() const;

    /** Return the preferred encoding from the offered list.

        @par Example
        @code
        // Accept-Encoding: gzip, deflate
        accepts ac( req );
        ac.encoding({ "gzip", "deflate" }); // => "gzip"
        @endcode

        @param offered The encodings to negotiate from.

        @return The best matching encoding, or an empty
        string view if none are acceptable.
    */
    std::string_view
    encoding(
        std::initializer_list<
            std::string_view> offered ) const;

    /** Return all accepted encodings sorted by preference.

        @return A vector of accepted encodings.
    */
    std::vector<std::string_view>
    encodings() const;

    /** Return the preferred charset from the offered list.

        @param offered The charsets to negotiate from.

        @return The best matching charset, or an empty
        string view if none are acceptable.
    */
    std::string_view
    charset(
        std::initializer_list<
            std::string_view> offered ) const;

    /** Return all accepted charsets sorted by preference.

        @return A vector of accepted charsets.
    */
    std::vector<std::string_view>
    charsets() const;

    /** Return the preferred language from the offered list.

        @param offered The languages to negotiate from.

        @return The best matching language, or an empty
        string view if none are acceptable.
    */
    std::string_view
    language(
        std::initializer_list<
            std::string_view> offered ) const;

    /** Return all accepted languages sorted by preference.

        @return A vector of accepted languages.
    */
    std::vector<std::string_view>
    languages() const;
};

} // http
} // boost

#endif
