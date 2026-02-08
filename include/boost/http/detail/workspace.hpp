//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_WORKSPACE_HPP
#define BOOST_HTTP_DETAIL_WORKSPACE_HPP

#include <boost/http/detail/except.hpp>
#include <boost/assert.hpp>
#include <cstdlib>
#include <new>
#include <utility>
#include <stddef.h> // ::max_align_t

namespace boost {
namespace http {
namespace detail {

/** A contiguous buffer of storage used by algorithms.

    Objects of this type retain ownership of a
    contiguous buffer of storage allocated upon
    construction. This storage is divided into
    three regions:

    @code
    | front | free | acquired | back |
    @endcode

    @li The reserved area, which starts at the
        beginning of the buffer and can grow
        upwards towards the end of the buffer.

    @li The acquired area, which starts at the
        end of the buffer and can grow downwards
        towards the beginning of the buffer.

    @li The unused area, which starts from the
        end of the reserved area and stretches
        until the beginning of the acquired area.
*/
class workspace
{
    unsigned char* begin_ = nullptr;
    unsigned char* front_ = nullptr;
    unsigned char* head_ = nullptr;
    unsigned char* back_ = nullptr;
    unsigned char* end_ = nullptr;

    template<class>
    struct any_impl;
    struct any;
    struct undo;

public:
    /** Return the number of aligned bytes required for T
    */
    template<class T>
    static
    constexpr
    std::size_t
    space_needed();

    /** Destructor.
    */
    ~workspace();

    /** Constructor.

        @param n The number of bytes of storage
            to allocate for the internal buffer.
    */
    explicit
    workspace(
        std::size_t n);

    /** Constructor.
    */
    workspace() = default;

    /** Constructor.
    */
    workspace(workspace&&) noexcept;

    /** Assignment.
    */
    workspace&
    operator=(workspace&&) noexcept;

    /** Allocate internal storage.

        @throw std::logic_error this->size() > 0

        @throw std::invalid_argument n == 0
    */
    void
    allocate(
        std::size_t n);

    /** Return a pointer to the unused area.
    */
    unsigned char*
    data() noexcept
    {
        return front_;
    }

    /** Return the size of the unused area.
    */
    std::size_t
    size() const noexcept
    {
        return head_ - front_;
    }

    /** Clear the contents while preserving capacity.
    */
    BOOST_HTTP_DECL
    void
    clear() noexcept;

    /** Convert unused storage to reserved storage.

        @throw std::invalid_argument n >= this->size()
    */
    BOOST_HTTP_DECL
    unsigned char*
    reserve_front(
        std::size_t n);

    /** Convert unused storage to reserved storage.

        @return nullptr if n >= this->size() 
    */
    BOOST_HTTP_DECL
    unsigned char*
    try_reserve_front(
        std::size_t n) noexcept;

    template<class T, class... Args>
    typename std::decay<T>::type&
    emplace(Args&&... args);

    template<class T>
    T*
    push_array(
        std::size_t n,
        T const& t);

    BOOST_HTTP_DECL
    unsigned char*
    reserve_back(
        std::size_t n);

private:
    BOOST_HTTP_DECL
    unsigned char*
    bump_down(
        std::size_t size,
        std::size_t align);
};

} // detail
} // http
} // boost

#include <boost/http/detail/impl/workspace.hpp>

#endif
