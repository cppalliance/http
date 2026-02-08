//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/detail/workspace.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/assert.hpp>
#include <boost/core/exchange.hpp>
#include <utility>

namespace boost {
namespace http {
namespace detail {

workspace::
any::
~any() = default;

workspace::
~workspace()
{
    clear();
    delete[] begin_;
}

workspace::
workspace(
    std::size_t n)
    : begin_(new unsigned char[n])
    , front_(begin_)
    , head_(begin_ + n)
    , back_(head_)
    , end_(head_)
{
}

workspace::
workspace(
    workspace&& other) noexcept
    : begin_(boost::exchange(other.begin_, nullptr))
    , front_(boost::exchange(other.front_, nullptr))
    , head_(boost::exchange(other.head_, nullptr))
    , back_(boost::exchange(other.back_, nullptr))
    , end_(boost::exchange(other.end_, nullptr))
{
}

workspace&
workspace::
operator=(
    workspace&& other) noexcept
{
    if(this != &other)
    {
        delete[] begin_;

        begin_ = boost::exchange(other.begin_, nullptr);
        front_ = boost::exchange(other.front_, nullptr);
        head_  = boost::exchange(other.head_, nullptr);
        back_  = boost::exchange(other.back_, nullptr);
        end_   = boost::exchange(other.end_, nullptr);
    }
    return *this;
}

void
workspace::
allocate(
    std::size_t n)
{
    // Cannot be empty
    if(n == 0)
        detail::throw_invalid_argument();

    // Already allocated
    if(begin_ != nullptr)
        detail::throw_logic_error();

    begin_ = new unsigned char[n];
    front_ = begin_;
    head_ = begin_ + n;
    back_ = head_;
    end_ = head_;
}

void
workspace::
clear() noexcept
{
    if(! begin_)
        return;

    auto const end =
        reinterpret_cast<
            any const*>(back_);
    auto p =
        reinterpret_cast<
            any const*>(head_);
    while(p != end)
    {
        auto next = p->next;
        p->~any();
        p = next;
    }
    front_ = begin_;
    head_ = end_;
    back_ = end_;
}

unsigned char*
workspace::
reserve_front(
    std::size_t n)
{
    // Requested size exceeds available space.
    // Note you can never reserve the last byte.
    if(n >= size())
        detail::throw_length_error();

    auto const p = front_;
    front_ += n ;
    return p;
}

unsigned char*
workspace::
try_reserve_front(
    std::size_t n) noexcept
{
    // Requested size exceeds available space.
    // Note you can never reserve the last byte.
    if(n >= size())
        return nullptr;

    auto const p = front_;
    front_ += n ;
    return p;
}

unsigned char*
workspace::
reserve_back(
    std::size_t n)
{
    // // can't reserve after acquire
    // if(head_ != end_)
    //     detail::throw_logic_error();

    // can't reserve twice
    if(back_ != end_)
        detail::throw_logic_error();

    // over capacity
    std::size_t const lim =
        head_ - front_;
    if(n >= lim)
        detail::throw_length_error();

    head_ -= n;
    back_ = head_;
    return back_;
}

// https://fitzgeraldnick.com/2019/11/01/always-bump-downwards.html
unsigned char*
workspace::
bump_down(
    std::size_t size,
    std::size_t align)
{
    BOOST_ASSERT(align > 0);
    BOOST_ASSERT(
        (align & (align - 1)) == 0);

    auto ip0 = reinterpret_cast<
        std::uintptr_t>(front_);
    auto ip = reinterpret_cast<
        std::uintptr_t>(head_);

    // If you get an exception here, it
    // means that a buffer was too small
    // for your workload. Increase the
    // buffer size.
    if(size > ip - ip0)
        detail::throw_length_error();

    ip -= size;
    ip &= ~(align - 1);

    // If you get an exception here, it
    // means that a buffer was too small
    // for your workload. Increase the
    // buffer size.
    if(ip < ip0)
        detail::throw_length_error();

    return reinterpret_cast<
        unsigned char*>(ip);
}

} // detail
} // http
} // boost
