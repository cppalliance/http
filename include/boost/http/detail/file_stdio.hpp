//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_FILE_STDIO_HPP
#define BOOST_HTTP_DETAIL_FILE_STDIO_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/error.hpp>
#include <boost/http/file_mode.hpp>
#include <cstdio>
#include <cstdint>

namespace boost {
namespace http {
namespace detail {

// Implementation of File which uses cstdio.
class file_stdio
{
    std::FILE* f_ = nullptr;

public:
    using native_handle_type = std::FILE*;

    BOOST_HTTP_DECL
    ~file_stdio();

    file_stdio() = default;

    BOOST_HTTP_DECL
    file_stdio(file_stdio&& other) noexcept;

    BOOST_HTTP_DECL
    file_stdio&
    operator=(file_stdio&& other) noexcept;

    std::FILE*
    native_handle() const
    {
        return f_;
    }

    BOOST_HTTP_DECL
    void
    native_handle(std::FILE* f);

    bool
    is_open() const
    {
        return f_ != nullptr;
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
