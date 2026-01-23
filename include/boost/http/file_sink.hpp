//
// Copyright (c) 2022 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_FILE_SINK_HPP
#define BOOST_HTTP_FILE_SINK_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/sink.hpp>
#include <boost/capy/file.hpp>

namespace boost {
namespace http {

/** Writes a message body to a file.

    This class implements the @ref sink interface,
    enabling message bodies to be written directly
    to a file. It is typically used with @ref parser
    to handle large payloads efficiently.

    @par Example
    @code
    parser.set_body<file_sink>("example.zip", file_mode::write_new);
    @endcode

    @see
        @ref file,
        @ref parser,
        @ref sink.
*/
class file_sink
    : public sink
{
    capy::file f_;

public:
    /** Constructor.

        @param f An open @ref file object that
        will receive the body data.
    */
    BOOST_HTTP_DECL
    explicit
    file_sink(capy::file&& f) noexcept;

    file_sink() = delete;
    file_sink(file_sink const&) = delete;

    /** Constructor.
    */
    BOOST_HTTP_DECL
    file_sink(file_sink&&) noexcept;

    /** Destructor.
    */
    BOOST_HTTP_DECL
    ~file_sink();

private:
    BOOST_HTTP_DECL
    results
    on_write(
        capy::const_buffer, bool) override;
};

} // http
} // boost

#endif
