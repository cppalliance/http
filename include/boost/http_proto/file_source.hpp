//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_FILE_SOURCE_HPP
#define BOOST_HTTP_PROTO_FILE_SOURCE_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/source.hpp>
#include <boost/capy/file.hpp>
#include <cstdint>

namespace boost {
namespace http_proto {

/** Reads a message body from a file.

    This class implements the @ref source interface
    and can be used with a @ref serializer to send
    the contents of a file as the HTTP message body.

    @par Example
    @code
    file f("example.zip", file_mode::scan);
    response.set_payload_size(f.size());
    serializer.start<file_source>(response, std::move(f));
    @endcode

    @see
        @ref file_sink,
        @ref file,
        @ref serializer,
        @ref source.
*/
class file_source
    : public source
{
    capy::file f_;
    std::uint64_t n_;

public:
    /** Constructor.

        @param f An open @ref file from which the
        body will be read.

        @param limit An upper bound on the number
        of bytes to read from the file. If `limit`
        exceeds the size of the file, the entire
        file will be read.
    */
    BOOST_HTTP_PROTO_DECL
    file_source(
        capy::file&& f,
        std::uint64_t limit =
            std::uint64_t(-1)) noexcept;

    file_source() = delete;
    file_source(file_source const&) = delete;

    /** Constructor.
    */
    BOOST_HTTP_PROTO_DECL
    file_source(file_source&&) noexcept;

    /** Destructor.
    */
    BOOST_HTTP_PROTO_DECL
    ~file_source();

private:
    BOOST_HTTP_PROTO_DECL
    results
    on_read(
        buffers::mutable_buffer b) override;
};

} // http_proto
} // boost

#endif
