//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Christian Mazakas
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RESPONSE_BASE_HPP
#define BOOST_HTTP_RESPONSE_BASE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/message_base.hpp>
#include <boost/http/status.hpp>

namespace boost {
namespace http {

/** Mixin for modifing HTTP responses.

    @see
        @ref message_base,
        @ref response,
        @ref static_response.
*/
class response_base
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
    http::status
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
        http::status sc,
        http::version v =
            http::version::http_1_1)
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
    BOOST_HTTP_DECL
    void
    set_version(
        http::version v);

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
        http::status sc)
    {
        if(sc == http::status::unknown)
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
        http::version v =
            http::version::http_1_1)
    {
        set_start_line_impl(
            int_to_status(si),
            si,
            reason,
            v);
    }

private:
    BOOST_HTTP_DECL
    void
    set_start_line_impl(
        http::status sc,
        unsigned short si,
        core::string_view reason,
        http::version v);
};

} // http
} // boost

#endif
