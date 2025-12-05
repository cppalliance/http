//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Christian Mazakas
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_RESPONSE_BASE_HPP
#define BOOST_HTTP_PROTO_RESPONSE_BASE_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/message_base.hpp>
#include <boost/http_proto/status.hpp>

namespace boost {
namespace http_proto {

/** Mixin for modifing HTTP responses.

    @see
        @ref message_base,
        @ref response,
        @ref static_response.
*/
class BOOST_HTTP_PROTO_DECL response_base
    : public message_base
{
    friend class response;
    friend class static_response;

    response_base() noexcept
        : message_base(detail::kind::response)
    {
    }

    explicit
    response_base(core::string_view s)
        : message_base(detail::kind::response, s)
    {
    }

    response_base(
        void* storage,
        std::size_t cap) noexcept
        : message_base(
            detail::kind::response, storage, cap)
    {
    }

public:
    //--------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------

    /** Return the reason string.

        This field is obsolete in HTTP/1
        and should only be used for display
        purposes.
    */
    core::string_view
    reason() const noexcept
    {
        return core::string_view(
            h_.cbuf + 13,
            h_.prefix - 15);
    }

    /** Return the status code.
    */
    http_proto::status
    status() const noexcept
    {
        return h_.res.status;
    }

    /** Return the status code as an integral.
    */
    unsigned short
    status_int() const noexcept
    {
        return h_.res.status_int;
    }

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    /** Set the status code and version of the response.

        The reason-phrase will be set to the
        standard text for the specified status
        code.

        This is more efficient than setting the
        properties individually.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.
        @throw std::invalid_argument
        `sc == status::unknown`

        @param sc The status code to set. This
        must not be @ref status::unknown.

        @param v The version to set.
    */
    void
    set_start_line(
        http_proto::status sc,
        http_proto::version v =
            http_proto::version::http_1_1)
    {
        set_start_line_impl(sc,
            static_cast<unsigned short>(sc),
                to_string(sc), v);
    }

    /** Set the HTTP version of the response

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if maximum capacity exceeded.

        @throw std::length_error
        Maximum capacity would be exceeded.

        @param v The version to set.
    */
    BOOST_HTTP_PROTO_DECL
    void
    set_version(
        http_proto::version v);

    /** Set the status code of the response.

        The reason-phrase will be set to the
        standard text for the specified status
        code. The version will remain unchanged.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if maximum capacity exceeded.

        @throw std::length_error
        Maximum capacity would be exceeded.
        @throw std::invalid_argument
        `sc == status::unknown`

        @param sc The status code to set. This
        must not be @ref status::unknown.
    */
    void
    set_status(
        http_proto::status sc)
    {
        if(sc == http_proto::status::unknown)
            detail::throw_invalid_argument();
        set_start_line_impl(sc,
            static_cast<unsigned short>(sc),
            to_string(sc),
            version());
    }

    /** Set the status code and version of the response.

        The reason-phrase will be set to the
        standard text for the specified status
        code.

        This is more efficient than setting the
        properties individually.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param si An integral representing the
        status code to set.

        @param reason A string view representing the
        reason string to set.

        @param v The version to set.
    */
    void
    set_start_line(
        unsigned short si,
        core::string_view reason,
        http_proto::version v =
            http_proto::version::http_1_1)
    {
        set_start_line_impl(
            int_to_status(si),
            si,
            reason,
            v);
    }

private:
    
    void
    set_start_line_impl(
        http_proto::status sc,
        unsigned short si,
        core::string_view reason,
        http_proto::version v);
};

} // http_proto
} // boost

#endif
