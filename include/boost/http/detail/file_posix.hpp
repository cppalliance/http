//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_FILE_POSIX_HPP
#define BOOST_HTTP_DETAIL_FILE_POSIX_HPP

#include <boost/http/detail/config.hpp>

#if ! defined(BOOST_HTTP_NO_POSIX_FILE)
# if ! defined(__APPLE__) && ! defined(__linux__) && ! defined(__FreeBSD__) && ! defined(__NetBSD__)
#  define BOOST_HTTP_NO_POSIX_FILE
# endif
#endif

#if ! defined(BOOST_HTTP_USE_POSIX_FILE)
# if ! defined(BOOST_HTTP_NO_POSIX_FILE)
#  define BOOST_HTTP_USE_POSIX_FILE 1
# else
#  define BOOST_HTTP_USE_POSIX_FILE 0
# endif
#endif

#if BOOST_HTTP_USE_POSIX_FILE

#include <boost/http/error.hpp>
#include <boost/http/file_mode.hpp>
#include <boost/system/error_code.hpp>
#include <cstdint>

namespace boost {
namespace http {
namespace detail {

// Implementation of File for POSIX systems.
class file_posix
{
    int fd_ = -1;

    BOOST_HTTP_DECL
    static
    int
    native_close(int& fd);

public:
    using native_handle_type = int;

    BOOST_HTTP_DECL
    ~file_posix();

    file_posix() = default;

    BOOST_HTTP_DECL
    file_posix(file_posix&& other) noexcept;

    BOOST_HTTP_DECL
    file_posix&
    operator=(file_posix&& other) noexcept;

    native_handle_type
    native_handle() const
    {
        return fd_;
    }

    BOOST_HTTP_DECL
    void
    native_handle(native_handle_type fd);

    bool
    is_open() const
    {
        return fd_ != -1;
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
