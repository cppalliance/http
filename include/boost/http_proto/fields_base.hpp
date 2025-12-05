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

#ifndef BOOST_HTTP_PROTO_FIELDS_BASE_HPP
#define BOOST_HTTP_PROTO_FIELDS_BASE_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/detail/except.hpp>
#include <boost/http_proto/detail/header.hpp>
#include <boost/core/detail/string_view.hpp>

#include <iosfwd>

namespace boost {
namespace http_proto {

/** Mixin for modifiable HTTP fields.

    @par Iterators

    Iterators obtained from @ref fields
    containers are not invalidated when
    the underlying container is modified.

    @note HTTP field names are case-insensitive.
*/
class BOOST_HTTP_PROTO_DECL fields_base
{
    detail::header h_;
    std::size_t max_cap_ =
        std::numeric_limits<std::size_t>::max();
    bool external_storage_ = false;

    using entry =
        detail::header::entry;
    using offset_type =
        detail::header::offset_type;
    using table =
        detail::header::table;

    class op_t;
    class prefix_op_t
    {
        fields_base& self_;
        offset_type new_prefix_;
        char* buf_ = nullptr;

    public:
        prefix_op_t(
            fields_base& self,
            std::size_t new_prefix,
            core::string_view* s0 = nullptr,
            core::string_view* s1 = nullptr);

        ~prefix_op_t();
    };

    friend class fields;
    friend class message_base;
    friend class request_base;
    friend class request;
    friend class static_request;
    friend class response_base;
    friend class response;
    friend class static_response;
    friend class parser;
    friend class serializer;


    explicit
    fields_base(
        detail::kind k) noexcept;


    fields_base(
        detail::kind k,
        void* storage,
        std::size_t cap) noexcept;


    fields_base(
        detail::kind k,
        core::string_view s);


    explicit
    fields_base(
        detail::header const& h);


    fields_base(
        fields_base const&);


    fields_base(
        detail::header const& h,
        void* storage,
        std::size_t cap);

public:
    //--------------------------------------------
    //
    // Types
    //
    //--------------------------------------------

    /** A view to an HTTP field.

        The view will be invalidated when the
        underlying container is modified.

        The caller is responsible for ensuring
        that the lifetime of the container extends
        until it is no longer referenced.
    */
    struct reference
    {
        /** Field name constant.

            Set to `boost::none` if the constant
            does not exist in @ref field.
        */
        boost::optional<field> const id;

        /// A view to the field name.
        core::string_view const name;
        
        /// A view to the field value.
        core::string_view const value;

        reference const*
        operator->() const noexcept
        {
            return this;
        }
    };

    /// @copydoc reference
    typedef reference const_reference;

    /** A value type which represent an HTTP field.

        This type allows for making a copy of
        a field where ownership is retained
        in the copy.
    */
    struct value_type
    {
        /** Field name constant.

            Set to `boost::none` if the
            constant does not exist in @ref field.
        */
        boost::optional<field> id;

        /// Field name.
        std::string name;

        /// Field value.
        std::string value;

        /// Constructor.

        value_type(
            reference const& other);

        /** Conversion.

            @see
                @ref reference.

            @return A view to the fields.
        */
        operator reference() const noexcept;
    };

    /** A bidirectional iterator to HTTP fields.
    */
    class iterator;

    /// @copydoc iterator
    using const_iterator = iterator;

    /** A bidirectional reverse iterator to HTTP fields.
    */
    class reverse_iterator;

    /// @copydoc iterator
    using const_reverse_iterator = reverse_iterator;

    /** A forward range of matching fields.

        Objects of this type are returned by
        the function @ref find_all.
    */
    class subrange;

    //--------------------------------------------
    //
    // Special Members
    //
    //--------------------------------------------

    /** Destructor.
    */

    ~fields_base();

    //--------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------

    /** Return the largest possible serialized message.
    */
    static
    constexpr
    std::size_t
    max_size() noexcept
    {
        // TODO: this doesn't take into account
        // the start-line
        return detail::header::max_offset;
    }

    /** Return an iterator to the beginning.
    */
    iterator
    begin() const noexcept;

    /** Return an iterator to the end.
    */
    iterator
    end() const noexcept;

    /** Return a reverse iterator to the beginning.
    */
    reverse_iterator
    rbegin() const noexcept;

    /** Return a reverse iterator to the end.
    */
    reverse_iterator
    rend() const noexcept;

    /** Return a string view representing the serialized data.
    */
    core::string_view
    buffer() const noexcept
    {
        return core::string_view(
            h_.cbuf, h_.size);
    }

    /** Return the number of fields in the container.
    */
    std::size_t
    size() const noexcept
    {
        return h_.count;
    }

    /** Return the value of a field, or throws an exception.

        If more than one field with the specified
        name exists, the first field defined by
        insertion order is returned.

        @par Exception Safety
        Strong guarantee.

        @throw std::out_of_range
        Field is not found.

        @param id The field name constant.
    */

    core::string_view
    at(field id) const;

    /** Return the value of a field, or throws an exception.

        If more than one field with the specified
        name exists, the first field defined by
        insertion order is returned.

        If `name` refers to a known field, it is
        faster to call @ref at with a field id
        instead of a string.

        @par Exception Safety
        Strong guarantee.

        @throw std::out_of_range
        Field is not found.

        @param name The field name.
    */

    core::string_view
    at(core::string_view name) const;

    /** Return true if a field exists.
    */

    bool
    exists(field id) const noexcept;

    /** Return true if a field exists.

        If `name` refers to a known field,
        it is faster to call @ref exists
        with a field id instead of a string.

        @param name The field name.
    */

    bool
    exists(
        core::string_view name) const noexcept;

    /** Return the number of matching fields.

        @param id The field name constant.
    */

    std::size_t
    count(field id) const noexcept;

    /** Return the number of matching fields.

        If `name` refers to a known field,
        it is faster to call @ref count
        with a field id instead of a string.

        @param name The field name.
    */

    std::size_t
    count(
        core::string_view name) const noexcept;

    /** Return an iterator to the matching element if it exists.

        @param id The field name constant.
    */

    iterator
    find(field id) const noexcept;

    /** Return an iterator to the matching element if it exists.

        If `name` refers to a known field,
        it is faster to call @ref find
        with a field id instead of a string.

        @param name The field name.
    */

    iterator
    find(
        core::string_view name) const noexcept;

    /** Return an iterator to the matching element if it exists.

        @param from The position to begin the
        search from. This can be `end()`.

        @param id The field name constant.
    */

    iterator
    find(
        iterator from,
        field id) const noexcept;

    /** Return an iterator to the matching element if it exists.

        If `name` refers to a known field,
        it is faster to call @ref find
        with a field id instead of a string.

        @param from The position to begin the
        search from. This can be `end()`.

        @param name The field name.
    */

    iterator
    find(
        iterator from,
        core::string_view name) const noexcept;

    /** Return an iterator to the matching element if it exists.

        @param before One past the position
        to begin the search from. This can
        be `end()`.

        @param id The field name constant.
    */

    iterator
    find_last(
        iterator before,
        field id) const noexcept;

    /** Return an iterator to the matching element if it exists.

        If `name` refers to a known field,
        it is faster to call @ref find_last
        with a field id instead of a string.

        @param before One past the position
        to begin the search from. This can
        be `end()`.

        @param name The field name.
    */

    iterator
    find_last(
        iterator before,
        core::string_view name) const noexcept;

    /** Return the value of a field or a default if missing.

        @param id The field name constant.

        @param s The value to be returned if
        field does not exist.
    */

    core::string_view
    value_or(
        field id,
        core::string_view s) const noexcept;

    /** Return the value of a field or a default if missing.

        If `name` refers to a known field,
        it is faster to call @ref value_or
        with a field id instead of a string.

        @param name The field name.

        @param s The value to be returned if
        field does not exist.
    */

    core::string_view
    value_or(
        core::string_view name,
        core::string_view s) const noexcept;

    /** Return a forward range containing values for all matching fields.

        @param id The field name constant.
    */

    subrange
    find_all(field id) const noexcept;

    /** Return a forward range containing values for all matching fields.

        If `name` refers to a known field,
        it is faster to call @ref find_all
        with a field id instead of a string.

        @param name The field name.
    */

    subrange
    find_all(
        core::string_view name) const noexcept;

    //--------------------------------------------
    //
    // Capacity
    //
    //--------------------------------------------

    /** Return the maximum allowed capacity in bytes.
    */
    std::size_t
    max_capacity_in_bytes() noexcept
    {
        return max_cap_;
    }

    /** Return the total number of bytes allocated by the container.
    */
    std::size_t
    capacity_in_bytes() const noexcept
    {
        return h_.cap;
    }

    /** Clear contents while preserving the capacity.

        In the case of response and request
        containers the start-line also resets to
        default.

        @par Postconditions
        @code
        this->size() == 0
        @endcode

        @par Complexity
        Constant.
    */

    void
    clear() noexcept;

    /** Adjust the capacity without changing the size.

        This function adjusts the capacity
        of the container in bytes, without
        affecting the current contents. Has
        no effect if `n <= this->capacity_in_bytes()`.

        @par Postconditions
        @code
        this->capacity_in_bytes() >= n
        @endcode

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param n The capacity in bytes.
    */

    void
    reserve_bytes(std::size_t n);

    /** Set the maximum allowed capacity in bytes.

        Prevents the container from growing beyond
        `n` bytes. Exceeding this limit will throw
        an exception.

        @par Preconditions
        @code
        this->capacity_in_bytes() <= n
        @endcode

        @par Postconditions
        @code
        this->max_capacity_in_bytes() == n
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exception thrown on invalid input.

        @throw std::invalid_argument
        `n < this->capacity_in_bytes()`

        @param n The maximum allowed capacity in bytes.
    */

    void
    set_max_capacity_in_bytes(std::size_t n);

    /** Remove excess capacity.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
    */

    void
    shrink_to_fit();

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    /** Append a header.

        This function appends a new header.
        Existing headers with the same name are
        not changed.

        Any leading or trailing whitespace in the
        value is ignored.

        No iterators are invalidated.

        @par Example
        @code
        request req;

        req.append( field::user_agent, "Boost" );
        @endcode

        @par Complexity
        Linear in `to_string( id ).size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param id The field name constant.

        @param value The value which must be semantically
        valid for the message.
    */
    void
    append(
        field id,
        core::string_view value)
    {
        system::error_code ec;
        append(id, value, ec);
        if(ec.failed())
            detail::throw_system_error(ec);
    }

    /** Append a header.

        This function appends a new header.
        Existing headers with the same name are
        not changed.

        Any leading or trailing whitespace in the
        value is ignored.

        No iterators are invalidated.

        @par Example
        @code
        request req;

        req.append( field::user_agent, "Boost" );
        @endcode

        @par Complexity
        Linear in `to_string( id ).size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param id The field name constant.

        @param value The value which must be semantically
        valid for the message.

        @param ec Set to the error if input is invalid.
    */
    void
    append(
        field id,
        core::string_view value,
        system::error_code& ec)
    {
        insert_impl(
            id,
            to_string(id),
            value,
            h_.count,
            ec);
    }

    /** Append a header.

        This function appends a new header.
        Existing headers with the same name are
        not changed.

        Any leading or trailing whitespace in the
        value is ignored.

        No iterators are invalidated.

        @par Example
        @code
        request req;

        req.append( "User-Agent", "Boost" );
        @endcode

        @par Complexity
        Linear in `name.size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param name The header name.

        @param value The header value, which must
        be semantically valid for the message.
    */
    void
    append(
        core::string_view name,
        core::string_view value)
    {
        system::error_code ec;
        append(name, value, ec);
        if(ec.failed())
            detail::throw_system_error(ec);
    }

    /** Append a header.

        This function appends a new header.
        Existing headers with the same name are
        not changed.

        Any leading or trailing whitespace in the
        value is ignored.

        No iterators are invalidated.

        @par Example
        @code
        request req;

        req.append( "User-Agent", "Boost" );
        @endcode

        @par Complexity
        Linear in `name.size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param name The header name.

        @param value The value which must be semantically
        valid for the message.

        @param ec Set to the error if input is invalid.
    */
    void
    append(
        core::string_view name,
        core::string_view value,
        system::error_code& ec)
    {
        insert_impl(
            string_to_field(name),
            name,
            value,
            h_.count,
            ec);
    }

    /** Insert a header.

        If a matching header with the same name
        exists, it is not replaced. Instead, an
        additional header with the same name is
        inserted. Names are not case-sensitive.
        Any leading or trailing whitespace in
        the new value is ignored.

        All iterators that are equal to `before`
        or come after are invalidated.

        @par Example
        @code
        request req;

        req.insert( req.begin(), field::user_agent, "Boost" );
        @endcode

        @par Complexity
        Linear in `to_string( id ).size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @return An iterator to the newly inserted header.

        @param before Position to insert before.

        @param id The field name constant.

        @param value The value which must be semantically
        valid for the message.
    */

    iterator
    insert(
        iterator before,
        field id,
        core::string_view value);

    /** Insert a header.

        If a matching header with the same name
        exists, it is not replaced. Instead, an
        additional header with the same name is
        inserted. Names are not case-sensitive.

        Any leading or trailing whitespace in
        the new value is ignored.

        All iterators that are equal to `before`
        or come after are invalidated.

        @par Example
        @code
        request req;

        req.insert( req.begin(), field::user_agent, "Boost" );
        @endcode

        @par Complexity
        Linear in `to_string( id ).size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @return An iterator to the newly inserted header.

        @param before Position to insert before.

        @param id The field name constant.

        @param value The value which must be semantically
        valid for the message.

        @param ec Set to the error if input is invalid.
    */

    iterator
    insert(
        iterator before,
        field id,
        core::string_view value,
        system::error_code& ec);

    /** Insert a header.

        If a matching header with the same name
        exists, it is not replaced. Instead, an
        additional header with the same name is
        inserted. Names are not case-sensitive.

        Any leading or trailing whitespace in
        the new value is ignored.

        All iterators that are equal to `before`
        or come after are invalidated.

        @par Example
        @code
        request req;

        req.insert( req.begin(), "User-Agent", "Boost" );
        @endcode

        @par Complexity
        Linear in `name.size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @return An iterator to the newly inserted header.

        @param before Position to insert before.

        @param name The header name.

        @param value The value which must be semantically
        valid for the message.
    */

    iterator
    insert(
        iterator before,
        core::string_view name,
        core::string_view value);

    /** Insert a header.

        If a matching header with the same name
        exists, it is not replaced. Instead, an
        additional header with the same name is
        inserted. Names are not case-sensitive.

        Any leading or trailing whitespace in
        the new value is ignored.

        All iterators that are equal to `before`
        or come after are invalidated.

        @par Example
        @code
        request req;

        req.insert( req.begin(), "User-Agent", "Boost" );
        @endcode

        @par Complexity
        Linear in `name.size() + value.size()`.

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @return An iterator to the newly inserted header.

        @param before Position to insert before.

        @param name The header name.

        @param value The value which must be semantically
        valid for the message.

        @param ec Set to the error if input is invalid.
    */

    iterator
    insert(
        iterator before,
        core::string_view name,
        core::string_view value,
        system::error_code& ec);

    //--------------------------------------------

    /** Erase headers.

        This function removes the header pointed
        to by `it`.

        All iterators that are equal to `it`
        or come after are invalidated.

        @par Complexity
        Linear in `name.size() + value.size()`.

        @return An iterator to one past the
        removed element.

        @param it The iterator to the element
        to erase.
    */

    iterator
    erase(iterator it) noexcept;

    /** Erase headers.

        This removes all headers whose name
        constant is equal to `id`.

        If any headers are erased, then all
        iterators equal to or that come after
        the first erased element are invalidated.
        Otherwise, no iterators are invalidated.

        @par Complexity
        Linear in `this->string().size()`.

        @return The number of headers erased.

        @param id The field name constant.
    */

    std::size_t
    erase(field id) noexcept;

    /** Erase all matching fields.

        This removes all headers with a matching
        name, using a case-insensitive comparison.

        If any headers are erased, then all
        iterators equal to or that come after
        the first erased element are invalidated.
        Otherwise, no iterators are invalidated.

        @par Complexity
        Linear in `this->string().size()`.

        @return The number of fields erased

        @param name The header name.
    */

    std::size_t
    erase(
        core::string_view name) noexcept;

    //--------------------------------------------

    /** Set a header value.

        Uses the given value to overwrite the
        current one in the header field pointed to
        by the iterator. The value must be
        syntactically valid or else an error is
        returned.

        Any leading or trailing whitespace in the
        new value is ignored.

        @par Complexity

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param it The iterator to the header.

        @param value The value which must be semantically
        valid for the message.
    */

    void
    set(iterator it, core::string_view value);

    /** Set a header value.

        Uses the given value to overwrite the
        current one in the header field pointed to
        by the iterator. The value must be
        syntactically valid or else an error is
        returned.

        Any leading or trailing whitespace in the
        new value is ignored.

        @par Complexity

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param it The iterator to the header.

        @param value The value which must be semantically
        valid for the message.

        @param ec Set to the error if input is invalid.
    */

    void
    set(
        iterator it,
        core::string_view value,
        system::error_code& ec);

    /** Set a header value.

        The container is modified to contain
        exactly one field with the specified id
        set to the given value, which must be
        syntactically valid or else an error is
        returned.

        Any leading or trailing whitespace in the
        new value is ignored.

        @par Postconditions
        @code
        this->count( id ) == 1 && this->at( id ) == value
        @endcode

        @par Complexity

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param id The field constant of the header
        to set.

        @param value The value which must be semantically
        valid for the message.
    */
    void
    set(
        field id,
        core::string_view value)
    {
        system::error_code ec;
        set(id, value, ec);
        if(ec.failed())
            detail::throw_system_error(ec);
    }

    /** Set a header value.

        The container is modified to contain
        exactly one field with the specified id
        set to the given value, which must be
        syntactically valid or else an error is
        returned.

        Any leading or trailing whitespace in the
        new value is ignored.

        @par Postconditions
        @code
        this->count( id ) == 1 && this->at( id ) == value
        @endcode

        @par Complexity

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param id The field name constant.

        @param value The value which must be semantically
        valid for the message.

        @param ec Set to the error if input is invalid.
    */

    void
    set(
        field id,
        core::string_view value,
        system::error_code& ec);

    /** Set a header value.

        The container is modified to contain
        exactly one field with the specified name
        set to the given value, which must be
        syntactically valid or else an error is
        returned.

        Any leading or trailing whitespace in the
        new value is ignored.

        @par Postconditions
        @code
        this->count( name ) == 1 && this->at( name ) == value
        @endcode

        @par Complexity

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown on invalid input.
        Exception thrown if max capacity exceeded.

        @throw system_error
        Input is invalid.

        @throw std::length_error
        Max capacity would be exceeded.

        @param name The field name.

        @param value The value which must be semantically
        valid for the message.
    */
    void
    set(
        core::string_view name,
        core::string_view value)
    {
        system::error_code ec;
        set(name, value, ec);
        if(ec.failed())
            detail::throw_system_error(ec);
    }

    /** Set a header value.

        The container is modified to contain
        exactly one field with the specified name
        set to the given value, which must be
        syntactically valid or else an error is
        returned.

        Any leading or trailing whitespace in the
        new value is ignored.

        @par Postconditions
        @code
        this->count( name ) == 1 && this->at( name ) == value
        @endcode

        @par Complexity

        @par Exception Safety
        Strong guarantee.
        Calls to allocate may throw.
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param name The field name.

        @param value The value which must be semantically
        valid for the message.

        @param ec Set to the error if input is invalid.
    */

    void
    set(
        core::string_view name,
        core::string_view value,
        system::error_code& ec);

    //--------------------------------------------

    /** Format the container to the output stream

        This function serializes the container to
        the specified output stream.

        @par Example
        @code
        request req;
        req.set(field::content_length, "42");
        std::stringstream ss;
        ss << req;
        assert( ss.str() == "GET / HTTP/1.1\nContent-Length: 42\n" );
        @endcode

        @par Effects
        @code
        return os << f.buffer();
        @endcode

        @par Complexity
        Linear in `f.buffer().size()`

        @par Exception Safety
        Basic guarantee.

        @return A reference to the output stream, for chaining

        @param os The output stream to write to.

        @param f The container to write.
    */
    friend

    std::ostream&
    operator<<(
        std::ostream& os,
        const fields_base& f);

private:

    void
    copy_impl(
        detail::header const&);


    void
    insert_impl(
        optional<field> id,
        core::string_view name,
        core::string_view value,
        std::size_t before,
        system::error_code& ec);

    void
    insert_unchecked(
        optional<field> id,
        core::string_view name,
        core::string_view value,
        std::size_t before,
        bool has_obs_fold);

    void
    raw_erase(
        std::size_t) noexcept;

    void
    raw_erase_n(field, std::size_t) noexcept;

    std::size_t
    erase_all(
        std::size_t i0,
        field id) noexcept;

    std::size_t
    erase_all(
        std::size_t i0,
        core::string_view name) noexcept;

    std::size_t
    offset(
        std::size_t i) const noexcept;

    std::size_t
    length(
        std::size_t i) const noexcept;
};

} // http_proto
} // boost

#include <boost/http_proto/impl/fields_base.hpp>

#endif
