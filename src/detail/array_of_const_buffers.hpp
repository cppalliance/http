//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_ARRAY_OF_BUFFERS_HPP
#define BOOST_HTTP_DETAIL_ARRAY_OF_BUFFERS_HPP

#include <boost/assert.hpp>
#include <boost/capy/buffers.hpp>

#include <cstdint>

namespace boost {
namespace http {
namespace detail {

class array_of_const_buffers
{
public:
    using value_type = capy::const_buffer;
    using iterator = value_type*;
    using const_iterator = iterator;

    array_of_const_buffers() = default;
    array_of_const_buffers(
        array_of_const_buffers const&) = default;
    array_of_const_buffers&
    operator=(array_of_const_buffers const&) = default;

    array_of_const_buffers(
        value_type* p,
        std::uint16_t n) noexcept;

    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    std::uint16_t
    size() const noexcept
    {
        return size_;
    }

    std::uint16_t
    max_size() const noexcept
    {
        return cap_;
    }

    std::uint16_t
    capacity() const noexcept
    {
        return cap_ - size_;
    }

    iterator
    begin() const noexcept
    {
        return base_ + pos_;
    }

    iterator
    end() const noexcept
    {
        return base_ + pos_ + size_;
    }

    value_type&
    operator[](
        std::uint16_t i) const noexcept
    {
        BOOST_ASSERT(i < cap_ - pos_);
        return base_[i + pos_];
    }

    void
    consume(std::size_t n);

    void
    reset(std::uint16_t n) noexcept;

    void
    slide_to_front() noexcept;

    void append(value_type) noexcept;

private:
    value_type* base_ = nullptr;
    std::uint16_t cap_ = 0;
    std::uint16_t pos_ = 0;
    std::uint16_t size_ = 0;
};

} // detail
} // http
} // boost

#endif
