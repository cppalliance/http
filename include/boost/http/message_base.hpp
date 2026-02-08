//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Christian Mazakas
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_MESSAGE_BASE_HPP
#define BOOST_HTTP_MESSAGE_BASE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/fields_base.hpp>
#include <boost/core/detail/string_view.hpp>

namespace boost {
namespace http {

/** Mixin for modifing common metadata
    in HTTP request and response messages.

    This type is useful for modifying common
    properties shared by both requests
    and responses.

    @see
        @ref response,
        @ref request,
        @ref static_response,
        @ref static_request,
        @ref metadata.
*/
class message_base
    : public fields_base
{
    friend class request_base;
    friend class response_base;

    using fields_base::fields_base;

public:
    //--------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------

    /** Return the type of payload of this message.
    */
    auto
    payload() const noexcept ->
        http::payload
    {
        return h_.md.payload;
    }

    /** Return the payload size.

        When @ref payload returns @ref payload::size,
        this function returns the number of octets
        in the actual message payload.

        @return The number of octets in the
        actual message payload.
    */
    std::uint64_t
    payload_size() const noexcept
    {
        BOOST_ASSERT(
            payload() == payload::size);
        return h_.md.payload_size;
    }

    /** Return true if semantics indicate
        connection persistence.
    */
    bool
    keep_alive() const noexcept
    {
        return h_.keep_alive();
    }

    /** Return metadata about the message.
    */
    auto
    metadata() const noexcept ->
        http::metadata const&
    {
        return h_.md;
    }

    /** Return true if the message is using a chunked
        transfer encoding.
    */
    bool
    chunked() const noexcept
    {
        return h_.md.transfer_encoding.is_chunked;
    }

    /** Return the HTTP-version.
    */
    http::version
    version() const noexcept
    {
        return h_.version;
    }

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    /** Set the payload size.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param n The payload size to set.
    */
    BOOST_HTTP_DECL
    void
    set_payload_size(
        std::uint64_t n);

    /** Set the Content-Length to the specified value.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param n The Content-Length to set.
    */
    BOOST_HTTP_DECL
    void
    set_content_length(
        std::uint64_t n);

    /** Set whether the payload is chunked.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param value The value to set.
    */
    BOOST_HTTP_DECL
    void
    set_chunked(bool value);

    /** Set whether the connection should stay open.

        Even when keep-alive is set to true, the
        semantics of the other header fields may
        require the connection to be closed. For
        example when there is no content length
        specified in a response.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param value The value to set.
    */
    BOOST_HTTP_DECL
    void
    set_keep_alive(bool value);
};

} // http
} // boost

#endif
