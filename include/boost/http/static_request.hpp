//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_STATIC_REQUEST_HPP
#define BOOST_HTTP_STATIC_REQUEST_HPP

#include <boost/http/request_base.hpp>

namespace boost {
namespace http {

/** A modifiable static container for HTTP requests.

    This container uses an external memory
    storage with fixed capacity.
    The contents may be inspected and modified,
    and the implementation maintains a useful
    invariant: changes to the request always
    leave it in a valid state.

    @par Example
    @code
    char buf[1024];
    static_request req(buf, sizeof(buf));

    req.set_start_line(method::get, "/");
    req.set(field::host, "example.com");
    req.set(field::accept_encoding, "gzip, deflate, br");
    req.set(field::cache_control, "no-cache");

    assert(req.buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Cache-Control: no-cache\r\n"
        "\r\n");
    @endcode

    @par Invariants
    @code
    this->capacity_in_bytes() == Capacity && this->max_capacity_in_bytes() ==
   Capacity
    @endcode

    @tparam Capacity The maximum capacity in bytes.

    @see
        @ref request,
        @ref request_base.
*/
class static_request
    : public request_base
{
public:
    //--------------------------------------------
    //
    // Special Members
    //
    //--------------------------------------------

    /** Constructor.

        Constructs a request object that uses an
        external memory storage and does not perform
        any allocations during its lifetime.

        The caller is responsible for ensuring that the
        lifetime of the storage extends until the
        request object is destroyed.

        @par Postcondition
        @code
        this->capacity_in_bytes() == cap
        this->max_capacity_in_bytes() == cap
        @endcode

        @param storage The storage to use.
        @param cap The capacity of the storage.
    */
    static_request(
        void* storage,
        std::size_t cap)
        : request_base(storage, cap)
    {
    }

    /** Constructor (deleted)
    */
    static_request(
        static_request const&) = delete;

    /** Constructor.

        The contents of `r` are transferred
        to the newly constructed object,
        which includes the underlying
        character buffer.
        After construction, the moved-from
        object has a valid but unspecified
        state where the only safe operation
        is destruction.

        @par Complexity
        Constant.

        @param r The request to move from.
    */
    static_request(
        static_request&& r) noexcept
        : request_base()
    {
        h_.swap(r.h_);
        external_storage_ = true;
        max_cap_ = r.max_cap_;
        r.max_cap_ = 0;
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
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param r The request to copy.

        @return A reference to this object.
    */
    static_request&
    operator=(
        static_request const& r)
    {
        copy_impl(r.h_);
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
        Exception thrown if max capacity exceeded.

        @throw std::length_error
        Max capacity would be exceeded.

        @param r The request to copy.

        @return A reference to this object.
    */
    static_request&
    operator=(
        request_base const& r)
    {
        copy_impl(r.h_);
        return *this;
    }
};

} // http
} // boost

#endif
