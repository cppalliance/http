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

#ifndef BOOST_HTTP_FIELDS_HPP
#define BOOST_HTTP_FIELDS_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/fields_base.hpp>

namespace boost {
namespace http {

/** A modifiable container of HTTP fields.

    This container owns a collection of HTTP
    fields, represented by a buffer which is
    managed by performing dynamic memory
    allocations as needed. The contents may be
    inspected and modified, and the implementation
    maintains a useful invariant: changes to the
    fields always leave it in a valid state.

    @par Example
    @code
    fields fs;

    fs.set(field::host, "example.com");
    fs.set(field::accept_encoding, "gzip, deflate, br");
    fs.set(field::cache_control, "no-cache");

    assert(fs.buffer() ==
        "Host: example.com\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Cache-Control: no-cache\r\n"
        "\r\n");
    @endcode

    @see
        @ref fields_base.
*/
class fields final
    : public fields_base
{
public:

    //--------------------------------------------
    //
    // Special Members
    //
    //--------------------------------------------

    /** Constructor.

        A default-constructed fields container
        contain no name-value pairs.

        @par Example
        @code
        fields fs;
        @endcode

        @par Postconditions
        @code
        this->buffer() == "\r\n"
        @endcode

        @par Complexity
        Constant.
    */
    fields() noexcept
        : fields_base(detail::kind::fields)
    {
    }


    /** Constructor.

        Constructs a fields container from the string
        `s`, which must contain valid HTTP headers or
        else an exception is thrown.
        The new fields container retains ownership by
        allocating a copy of the passed string.

        @par Example
        @code
        fields f(
            "Server: Boost.HTTP\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n"
            "Content-Length: 73\r\n"
            "\r\n");
        @endcode

        @par Postconditions
        @code
        this->buffer() == s && this->buffer().data() != s.data()
        @endcode

        @par Complexity
        Linear in `s.size()`.

        @par Exception Safety
        Calls to allocate may throw.
        Exception thrown on invalid input.

        @throw system_error
        Input is invalid.

        @param s The string to parse.
    */
    explicit
    fields(
        core::string_view s)
        : fields_base(detail::kind::fields, s)
    {
    }

    /** Constructor.

        Allocates `cap` bytes initially, with an
        upper limit of `max_cap`. Growing beyond
        `max_cap` will throw an exception.

        Useful when an estimated initial size is
        known, but further growth up to a
        maximum is allowed.

        @par Preconditions
        @code
        max_cap >= cap
        @endcode

        @par Exception Safety
        Calls to allocate may throw.
        Exception thrown on invalid input.

        @throw system_error
        Input is invalid.

        @param cap Initial capacity in bytes (may be `0`).

        @param max_cap Maximum allowed capacity in bytes.
    */
    explicit
    fields(
        std::size_t cap,
        std::size_t max_cap = std::size_t(-1))
        : fields()
    {
        reserve_bytes(cap);
        set_max_capacity_in_bytes(max_cap);
    }

    /** Constructor.

        The contents of `f` are transferred
        to the newly constructed object,
        which includes the underlying
        character buffer.
        After construction, the moved-from
        object is as if default-constructed.

        @par Postconditions
        @code
        f.buffer() == "\r\n"
        @endcode

        @par Complexity
        Constant.

        @param f The fields to move from.
    */
    fields(fields&& f) noexcept
        : fields_base(f.h_.kind)
    {
        swap(f);
    }


    /** Constructor.

        The newly constructed object contains
        a copy of `f`.

        @par Postconditions
        @code
        this->buffer() == f.buffer() && this->buffer().data() != f.buffer().data()
        @endcode

        @par Complexity
        Linear in `f.size()`.

        @par Exception Safety
        Calls to allocate may throw.

        @param f The fields to copy.
    */
    fields(fields const& f) = default;

    /** Assignment.

        The contents of `f` are transferred to
        `this`, including the underlying
        character buffer. The previous contents
        of `this` are destroyed.
        After assignment, the moved-from
        object is as if default-constructed.

        @par Postconditions
        @code
        f.buffer() == "\r\n"
        @endcode

        @par Complexity
        Constant.

        @param f The fields to assign from.

        @return A reference to this object.
    */
    fields&
    operator=(fields&& f) noexcept
    {
        fields tmp(std::move(f));
        tmp.swap(*this);
        return *this;
    }

    /** Assignment.

        The contents of `f` are copied and
        the previous contents of `this` are
        discarded.

        @par Postconditions
        @code
        this->buffer() == f.buffer() && this->buffer().data() != f.buffer().data()
        @endcode

        @par Complexity
        Linear in `f.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @return A reference to this object.

        @param f The fields to copy.
    */
    fields&
    operator=(fields const& f) noexcept
    {
        copy_impl(f.h_);
        return *this;
    }

    //--------------------------------------------

    /** Swap.

        Exchanges the contents of this fields
        object with another. All views, iterators
        and references remain valid.

        If `this == &other`, this function call has no effect.

        @par Example
        @code
        fields f1;
        f1.set(field::accept, "text/html");
        fields f2;
        f2.set(field::connection, "keep-alive");
        f1.swap(f2);
        assert(f1.buffer() == "Connection: keep-alive\r\n\r\n" );
        assert(f2.buffer() == "Accept: text/html\r\n\r\n" );
        @endcode

        @par Complexity
        Constant.

        @param other The object to swap with.
    */
    void
    swap(fields& other) noexcept
    {
        h_.swap(other.h_);
        std::swap(max_cap_, other.max_cap_);
    }

    /** Swap.

        Exchanges the contents of `v0` with
        another `v1`. All views, iterators and
        references remain valid.

        If `&v0 == &v1`, this function call has no effect.

        @par Example
        @code
        fields f1;
        f1.set(field::accept, "text/html");
        fields f2;
        f2.set(field::connection, "keep-alive");
        std::swap(f1, f2);
        assert(f1.buffer() == "Connection: keep-alive\r\n\r\n" );
        assert(f2.buffer() == "Accept: text/html\r\n\r\n" );
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
            @ref fields::swap.
    */
    friend
    void
    swap(
        fields& v0,
        fields& v1) noexcept
    {
        v0.swap(v1);
    }
};

} // http
} // boost

#endif
