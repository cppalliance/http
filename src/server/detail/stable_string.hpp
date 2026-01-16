//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_DETAIL_STABLE_STRING_HPP
#define BOOST_HTTP_SERVER_DETAIL_STABLE_STRING_HPP

#include <boost/http/detail/config.hpp>
#include <cstdint>
#include <cstdlib>

namespace boost {
namespace http {
namespace detail {

// avoids SBO
class stable_string
{
    char const* p_ = 0;
    std::size_t n_ = 0;

public:
    ~stable_string()
    {
        if(p_)
            delete[] p_;
    }

    stable_string() = default;

    stable_string(
        stable_string&& other) noexcept
        : p_(other.p_)
        , n_(other.n_)
    {
        other.p_ = nullptr;
        other.n_ = 0;
    }

    stable_string& operator=(
        stable_string&& other) noexcept
    {
        auto p = p_;
        auto n = n_;
        p_ = other.p_;
        n_ = other.n_;
        other.p_ = p;
        other.n_ = n;
        return *this;
    }

    explicit
    stable_string(
        std::string_view s)
        : p_(
            [&]
            {
                auto p =new char[s.size()];
                std::memcpy(p, s.data(), s.size());
                return p;
            }())
        , n_(s.size())
    {
    }

    stable_string(
        char const* it, char const* end)
        : stable_string(std::string_view(it, end))
    {
    }

    char const* data() const noexcept
    {
        return p_;
    }

    std::size_t size() const noexcept
    {
        return n_;
    }

    operator core::string_view() const noexcept
    {
        return { data(), size() };
    }

    operator std::string_view() const noexcept
    {
        return { data(), size() };
    }
};

} // detail
} // http
} // boost

#endif
