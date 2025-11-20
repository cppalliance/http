//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_RESPONSE_PARSER_HPP
#define BOOST_HTTP_PROTO_RESPONSE_PARSER_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/error.hpp>
#include <boost/http_proto/parser.hpp>
#include <boost/http_proto/static_response.hpp>
#include <boost/http_proto/status.hpp>

namespace boost {
namespace http_proto {

/// @copydoc parser
/// @brief A parser for HTTP/1 responses.
/// @see @ref request_parser.
class response_parser
    : public parser
{
public:
    /** Destructor.

        Any views or buffers obtained from this
        parser become invalid.
    */
    ~response_parser() = default;

    /** Constructor.

        Default-constructed parsers do not reference any
        implementation and therefore must be assigned to before using.
    */
    response_parser() = default;

    /** Constructor.

        Constructs a parser that uses the @ref
        config parameters installed on the
        provided `ctx`.

        The parser will attempt to allocate
        the required space on startup, with the
        amount depending on the @ref config
        parameters, and will not perform any
        further allocations, except for Brotli
        decoder instances, if enabled.

        Depending on which compression algorithms
        are enabled in the @ref config, the parser
        will attempt to access the corresponding
        decoder services on the same `ctx`.

        @par Example
        @code
        response_parser sr(ctx);
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Calls to allocate may throw.

        @param cfg Configuration settings.
    */
    BOOST_HTTP_PROTO_DECL
    explicit
    response_parser(prepared_parser_config cfg);

    /** Constructor.

        The states of `other` are transferred
        to the newly constructed object,
        including the allocated buffer.
        After construction, the only valid
        operations on the moved-from object
        are destruction and assignemt.

        Buffer sequences previously obtained
        using @ref prepare or @ref pull_body
        remain valid.

        @par Complexity
        Constant.

        @param other The parser to move from.
    */
    response_parser(
        response_parser&& other) noexcept = default;

    /** Assignment.
        The states of `other` are transferred
        to this object, including the allocated
        buffer.
        After assignment, the only valid
        operations on the moved-from object
        are destruction and assignment.
        Buffer sequences previously obtained
        using @ref prepare or @ref pull_body
        remain valid.
        @par Complexity
        Constant.
        @param other The parser to move from.
    */
    response_parser&
    operator=(response_parser&& other) noexcept
    {
        assign(std::move(other));
        return *this;
    }

    //-------------------------------------------

    /** Prepare for the next message on the stream.

        This informs the parser not to read a
        payload for the next message, regardless
        of the presence or absence of certain
        fields such as Content-Length or a chunked
        Transfer-Encoding. Depending on the request,
        some responses do not carry a body. For
        example, a 200 response to a CONNECT
        request from a tunneling proxy, or a
        response to a HEAD request. In these
        cases, callers may use this function
        inform the parser that no body is
        expected. The parser will consider the
        message complete after the header has
        been received.

        @par Preconditions
        No previous call to @ref start for the new message.

        @see
            https://datatracker.ietf.org/doc/html/rfc7230#section-3.3
    */
    void
    start_head_response()
    {
        start_impl(true);
    }

    /** Return a reference to the parsed response headers.

        The returned reference remains valid until:
        @li @ref start or @ref start_head_response is called
        @li @ref reset is called
        @li The parser instance is destroyed

        @par Preconditions
        @code
        this->got_header() == true
        @endcode

        @par Exception Safety
        Strong guarantee.

        @see
            @ref got_header.
    */
    BOOST_HTTP_PROTO_DECL
    static_response const&
    get() const;
};

} // http_proto
} // boost

#endif
