//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/detail/array_of_const_buffers.hpp"

#include <boost/http/detail/except.hpp>

#include <boost/capy/buffers/slice.hpp>

namespace boost {
namespace http {
namespace detail {

array_of_const_buffers::
array_of_const_buffers(
    value_type* p,
    std::uint16_t capacity) noexcept
    : base_(p)
    , cap_(capacity)
    , pos_(0)
    , size_(0)
{
}

void
array_of_const_buffers::
consume(std::size_t n)
{
    while(size_ > 0)
    {
        auto* p = base_ + pos_;
        if(n < p->size())
        {
            capy::remove_prefix(*p, n);
            return;
        }
        n -= p->size();
        ++pos_;
        --size_;
    }
}

void
array_of_const_buffers::
reset(std::uint16_t n) noexcept
{
    BOOST_ASSERT(n <= cap_);
    pos_  = 0;
    size_ = n;
}

void
array_of_const_buffers::
slide_to_front() noexcept
{
    auto* p = base_ + pos_; // begin
    auto* e = p + size_; // end
    auto* d = base_; // dest
    while(p < e)
        *d++ = *p++;
    pos_ = 0;
}

void
array_of_const_buffers::
append(value_type buf) noexcept
{
    BOOST_ASSERT(size_ < cap_);
    base_[pos_ + size_] = buf;
    ++size_;
}

} // detail
} // http
} // boost
