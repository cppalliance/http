//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_FILE_WIN32_HPP
#define BOOST_HTTP_DETAIL_FILE_WIN32_HPP

#include <boost/http/detail/config.hpp>

#if ! defined(BOOST_HTTP_USE_WIN32_FILE)
# ifdef _WIN32
#  define BOOST_HTTP_USE_WIN32_FILE 1
# else
#  define BOOST_HTTP_USE_WIN32_FILE 0
# endif
#endif

#if BOOST_HTTP_USE_WIN32_FILE

#include <boost/http/error.hpp>
#include <boost/http/file_mode.hpp>
#include <boost/winapi/handles.hpp>
#include <cstdint>

namespace boost {
namespace http {
namespace detail {

// Implementation of File for Win32.
class file_win32
{
    boost::winapi::HANDLE_ h_ =
        boost::winapi::INVALID_HANDLE_VALUE_;

public:
    using native_handle_type = boost::winapi::HANDLE_;

    BOOST_HTTP_DECL
    ~file_win32();

    file_win32() = default;

    BOOST_HTTP_DECL
    file_win32(file_win32&& other) noexcept;

    BOOST_HTTP_DECL
    file_win32&
    operator=(file_win32&& other) noexcept;

    native_handle_type
    native_handle()
    {
        return h_;
    }

    BOOST_HTTP_DECL
    void
    native_handle(native_handle_type h);

    bool
    is_open() const
    {
        return h_ != boost::winapi::INVALID_HANDLE_VALUE_;
    }

    BOOST_HTTP_DECL
    void
    close(system::error_code& ec);

    BOOST_HTTP_DECL
    void
    open(char const* path, file_mode mode, system::error_code& ec);

    BOOST_HTTP_DECL
    std::uint64_t
    size(system::error_code& ec) const;

    BOOST_HTTP_DECL
    std::uint64_t
    pos(system::error_code& ec) const;

    BOOST_HTTP_DECL
    void
    seek(std::uint64_t offset, system::error_code& ec);

    BOOST_HTTP_DECL
    std::size_t
    read(void* buffer, std::size_t n, system::error_code& ec);

    BOOST_HTTP_DECL
    std::size_t
    write(void const* buffer, std::size_t n, system::error_code& ec);
};

} // detail
} // http
} // boost

#endif

#endif
