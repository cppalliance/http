//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_REQUEST_BASE_HPP
#define BOOST_HTTP_PROTO_REQUEST_BASE_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/message_base.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace boost {
namespace http_proto {

/** Mixin for modifing HTTP requests.

    @see
        @ref message_base,
        @ref request,
        @ref static_request.
*/
class BOOST_HTTP_PROTO_DECL request_base
    : public message_base
{
    friend class request;
    friend class static_request;

    request_base() noexcept
        : message_base(detail::kind::request)
    {
    }

    explicit
    request_base(core::string_view s)
        : message_base(detail::kind::request, s)
    {
    }

    request_base(
        void* storage,
        std::size_t cap) noexcept
        : message_base(
            detail::kind::request, storage, cap)
    {
    }

public:
    //--------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------

    /** Return the method as a name constant.

        If the method returned is equal to
        @ref method::unknown, the method may
        be obtained as a string instead, by
        calling @ref method_text.
    */
    http_proto::method
    method() const noexcept
    {
        return h_.req.method;
    }

    /** Return the method as a string.
    */
    core::string_view
    method_text() const noexcept
    {
        return core::string_view(
            h_.cbuf,
            h_.req.method_len);
    }

    /** Return the request-target string.
    */
    core::string_view
    target() const noexcept
    {
        return core::string_view(
            h_.cbuf +
                h_.req.method_len + 1,
            h_.req.target_len);
    }

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    /** Set the method of the request to the enum.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param m The method to set.
    */
    void
    set_method(
        http_proto::method m)
    {
        set_start_line_impl(
            m,
            to_string(m),
            target(),
            version());
    }

    /** Set the method of the request to the string.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param s A string view representing the
        method to set.
    */
    void
    set_method(
        core::string_view s)
    {
        set_start_line_impl(
            string_to_method(s),
            s,
            target(),
            version());
    }

    /** Set the target string of the request.

        This function sets the request-target.
        The caller is responsible for ensuring
        that the string passed is syntactically
        valid.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param s A string view representing the
        target to set.
    */
    void
    set_target(
        core::string_view s)
    {
        set_start_line_impl(
            h_.req.method,
            method_text(),
            s,
            version());
    }

    /** Set the HTTP version of the request.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param v The version to set.
    */
    void
    set_version(
        http_proto::version v)
    {
        set_start_line_impl(
            h_.req.method,
            method_text(),
            target(),
            v);
    }

    /** Set the method, target, and version of the request.

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

        @param m The method to set.

        @param t A string view representing the
        target to set.

        @param v The version to set.
    */
    void
    set_start_line(
        http_proto::method m,
        core::string_view t,
        http_proto::version v =
            http_proto::version::http_1_1)
    {
        set_start_line_impl(m, to_string(m), t, v);
    }

    /** Set the method, target, and version of the request.

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

        @param m A string view representing the
        method to set.

        @param t A string view representing the
        target to set.

        @param v The version to set.
    */
    void
    set_start_line(
        core::string_view m,
        core::string_view t,
        http_proto::version v =
            http_proto::version::http_1_1)
    {
        set_start_line_impl(string_to_method(m), m, t, v);
    }

    /** Set the `Expect: 100-continue` header.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param b If `true` sets `Expect: 100-continue`
        header otherwise erase it.
    */
    void
    set_expect_100_continue(bool b);

private:
    
    void
    set_start_line_impl(
        http_proto::method m,
        core::string_view ms,
        core::string_view t,
        http_proto::version v);
};

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

} // http_proto
} // boost

#endif
