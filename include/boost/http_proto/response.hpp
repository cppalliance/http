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

#ifndef BOOST_HTTP_PROTO_RESPONSE_HPP
#define BOOST_HTTP_PROTO_RESPONSE_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/message_base.hpp>
#include <boost/http_proto/status.hpp>

namespace boost {
namespace http_proto {

/** A container for HTTP responses.

    This container owns a response, represented by
    a buffer which is managed by performing
    dynamic memory allocations as needed. The
    contents may be inspected and modified, and
    the implementation maintains a useful
    invariant: changes to the response always
    leave it in a valid state.

    @par Example
    @code
    response res(status::not_found);

    res.set(field::server, "Boost.HttpProto");
    res.set(field::content_type, "text/plain");
    res.set_content_length(80);

    assert(res.buffer() ==
        "HTTP/1.1 404 Not Found\r\n"
        "Server: Boost.HttpProto\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 80\r\n"
        "\r\n");
    @endcode

    @see
        @ref response_parser,
        @ref request.
*/
class response
    : public message_base
{
    friend class response_parser;

    response(
        view_tag_t,
        detail::header const& h) noexcept
        : message_base(view_tag, h)
    {
    }

public:
    //--------------------------------------------
    //
    // Special Members
    //
    //--------------------------------------------

    /** Constructor.

        A default-constructed response contains
        a valid HTTP 200 OK response with no headers.

        @par Example
        @code
        response res;
        @endcode

        @par Postconditions
        @code
        this->buffer() == "HTTP/1.1 200 OK\r\n\r\n"
        @endcode

        @par Complexity
        Constant.
    */
    response() noexcept
        : message_base(detail::kind::response)
    {
    }

    /** Constructor.

        Constructs a response from the string `s`,
        which must contain valid HTTP response
        or else an exception is thrown.
        The new response retains ownership by
        making a copy of the passed string.

        @par Example
        @code
        response res(
            "HTTP/1.1 404 Not Found\r\n"
            "Server: Boost.HttpProto\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n");
        @endcode

        @par Postconditions
        @code
        this->buffer.data() != s.data()
        @endcode

        @par Complexity
        Linear in `s.size()`.

        @par Exception Safety
        Calls to allocate may throw.
        Exception thrown on invalid input.

        @throw system_error
        The input does not contain a valid response.

        @param s The string to parse.
    */
    explicit
    response(
        core::string_view s)
        : message_base(detail::kind::response, s)
    {
    }

    /** Constructor.

        Allocates `cap` bytes initially, with an
        upper limit of `max_cap`. Growing beyond
        `max_cap` will throw an exception.

        Useful when an estimated initial size is
        known, but further growth up to a maximum
        is allowed.

        When `max_cap == cap`, the container
        guarantees to never allocate.

        @par Preconditions
        @code
        max_cap >= cap
        @endcode

        @par Exception Safety
        Calls to allocate may throw.

        @param cap Initial capacity in bytes (may be `0`).

        @param max_cap Maximum allowed capacity in bytes.
    */
    explicit
    response(
        std::size_t cap,
        std::size_t max_cap = std::size_t(-1))
        : response()
    {
        reserve_bytes(cap);
        set_max_capacity_in_bytes(max_cap);
    }

    /** Constructor.

        The start-line of the response will
        contain the standard text for the
        supplied status code and HTTP version.

        @par Example
        @code
        response res(status::not_found, version::http_1_0);
        @endcode

        @par Complexity
        Linear in `to_string(s).size()`.

        @par Exception Safety
        Calls to allocate may throw.

        @param sc The status code.

        @param v The HTTP version.
    */
    response(
        http_proto::status sc,
        http_proto::version v)
        : response()
    {
        set_start_line(sc, v);
    }

    /** Constructor.

        The start-line of the response will
        contain the standard text for the
        supplied status code with the HTTP version
        defaulted to `HTTP/1.1`.

        @par Example
        @code
        response res(status::not_found);
        @endcode

        @par Complexity
        Linear in `to_string(s).size()`.

        @par Exception Safety
        Calls to allocate may throw.

        @param sc The status code.
    */
    explicit
    response(
        http_proto::status sc)
        : response(
            sc, http_proto::version::http_1_1)
    {
    }

    /** Constructor.

        The contents of `r` are transferred
        to the newly constructed object,
        which includes the underlying
        character buffer.
        After construction, the moved-from
        object is as if default-constructed.

        @par Postconditions
        @code
        r.buffer() == "HTTP/1.1 200 OK\r\n\r\n"
        @endcode

        @par Complexity
        Constant.

        @param r The response to move from.
    */
    response(response&& r) noexcept
        : response()
    {
        swap(r);
    }

    /** Constructor.

        The newly constructed object contains
        a copy of `r`.

        @par Postconditions
        @code
        this->buffer() == r.buffer() && this->buffer.data() != r.buffer().data()
        @endcode

        @par Complexity
        Linear in `r.size()`.

        @par Exception Safety
        Calls to allocate may throw.

        @param r The response to copy.
    */
    response(response const&) = default;

    /** Assignment

        The contents of `r` are transferred to
        `this`, including the underlying
        character buffer. The previous contents
        of `this` are destroyed.
        After assignment, the moved-from
        object is as if default-constructed.

        @par Postconditions
        @code
        r.buffer() == "HTTP/1.1 200 OK\r\n\r\n"
        @endcode

        @par Complexity
        Constant.

        @param r The response to assign from.

        @return A reference to this object.
    */
    response&
    operator=(
        response&& r) noexcept
    {
        response temp(std::move(r));
        temp.swap(*this);
        return *this;
    }

    /** Assignment.

        The contents of `r` are copied and
        the previous contents of `this` are
        discarded.

        @par Postconditions
        @code
        this->buffer() == r.buffer() && this->buffer().data() != r.buffer().data()
        @endcode

        @par Complexity
        Linear in `r.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param r The response to copy.

        @return A reference to this object.
    */
    response&
    operator=(
        response const& r)
    {
        copy_impl(r.h_);
        return *this;
    }

    //--------------------------------------------

    /** Swap.

        Exchanges the contents of this response
        with another response. All views,
        iterators and references remain valid.

        If `this == &other`, this function call has no effect.

        @par Example
        @code
        response r1(status::ok);
        response r2(status::bad_request);
        r1.swap(r2);
        assert(r1.buffer() == "HTTP/1.1 400 Bad Request\r\n\r\n" );
        assert(r2.buffer() == "HTTP/1.1 200 OK\r\n\r\n" );
        @endcode

        @par Complexity
        Constant

        @param other The object to swap with
    */
    void
    swap(response& other) noexcept
    {
        h_.swap(other.h_);
        std::swap(max_cap_, other.max_cap_);
        std::swap(view_, other.view_);
    }

    /** Swap.

        Exchanges the contents of `v0` with
        another `v1`. All views, iterators and
        references remain valid.

        If `&v0 == &v1`, this function call has no effect.

        @par Example
        @code
        response r1(status::ok);
        response r2(status::bad_request);
        std::swap(r1, r2);
        assert(r1.buffer() == "HTTP/1.1 400 Bad Request\r\n\r\n" );
        assert(r2.buffer() == "HTTP/1.1 200 OK\r\n\r\n" );
        @endcode

        @par Effects
        @code
        v0.swap(v1);
        @endcode

        @par Complexity
        Constant.

        @param v0 The first object to swap.
        @param v1 The second object to swap.

        @see
            @ref response::swap.
    */
    friend
    void
    swap(
        response& v0,
        response& v1) noexcept
    {
        v0.swap(v1);
    }

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
    BOOST_HTTP_PROTO_DECL
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
