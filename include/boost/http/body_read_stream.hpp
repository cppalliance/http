//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_BODY_READ_STREAM_HPP
#define BOOST_HTTP_BODY_READ_STREAM_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/error.hpp>
#include <boost/http/parser.hpp>

#include <boost/assert.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/copy.hpp>
#include <boost/capy/cond.hpp>
#include <boost/capy/concept/read_stream.hpp>
#include <boost/capy/error.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <utility>

namespace boost {
namespace http {

/** A stream adapter for reading HTTP message bodies.

    This class wraps an underlying @ref capy::ReadStream and an HTTP
    @ref parser to provide a simple interface for reading message
    body data. The caller receives decoded body bytes; the parser
    automatically handles:

    @li HTTP header parsing (completed before body data is available)
    @li Chunked transfer-encoding (chunk framing removed automatically)
    @li Content-Encoding decompression (gzip, deflate, brotli if configured)
    @li Content-Length validation (if specified in headers)

    The class satisfies @ref capy::ReadStream, enabling composition
    with generic algorithms like @ref capy::read.

    @tparam Stream The underlying stream type satisfying
        @ref capy::ReadStream.

    @par Thread Safety
    Distinct objects: Safe.
    Shared objects: Unsafe.

    @par Example
    @code
    capy::task<void>
    receive_request(capy::ReadStream auto& socket)
    {
        http::polystore ctx;
        http::install_parser_service(ctx, http::request_parser::config{});
        http::request_parser pr(ctx);
        pr.reset();   // Initialize parser for new stream
        pr.start();   // Prepare to parse new message

        // Wrap socket + parser for body reading
        http::body_read_stream brs(socket, pr);

        // Read body data - chunking/decompression handled automatically
        char buf[1024];
        auto [ec, n] = co_await brs.read_some(
            capy::mutable_buffer(buf, sizeof(buf)));

        if (ec == capy::cond::eof)
        {
            // Body complete
        }
        else if (ec)
        {
            // Handle error
        }
        else
        {
            // Process n bytes in buf
        }
    }
    @endcode

    @par End of Body
    When the complete message body has been read, `read_some` returns
    `ec == capy::cond::eof` with `n == 0`. This indicates the body
    is complete and the parser's `is_complete()` returns `true`.

    @see capy::ReadStream, parser, request_parser, response_parser
*/
template<capy::ReadStream Stream>
class body_read_stream
{
    Stream& stream_;
    parser& pr_;

public:
    /** Constructor.

        @param s The underlying stream to read raw HTTP data from.
        @param pr The parser performing HTTP parsing. Must have been
            initialized with `reset()` and `start()` called before use.

        @note The parser's `got_header()` does not need to be true
            at construction time. The first `read_some` call will
            automatically read and parse headers if needed.
    */
    explicit
    body_read_stream(
        Stream& s,
        parser& pr)
        : stream_(s)
        , pr_(pr)
    {
    }

    /** Read body data from the message.

        Reads data from the underlying stream, parses it through
        the HTTP parser, and copies decoded body bytes into
        @p buffers.

        If headers have not yet been parsed, this function first
        reads and parses the complete HTTP headers before
        returning any body data.

        @tparam MB A type satisfying @ref capy::MutableBufferSequence.

        @param buffers The buffer sequence to read body data into.

        @return A task yielding `(error_code, bytes_transferred)`.
            On success, `bytes_transferred` is the number of body
            bytes copied into @p buffers.
            When the body is complete, returns `capy::cond::eof`
            with `bytes_transferred == 0`.
            On error, the error code indicates the failure.

        @par Example
        @code
        char buf[4096];
        auto [ec, n] = co_await brs.read_some(
            capy::mutable_buffer(buf, sizeof(buf)));
        @endcode
    */
    template<capy::MutableBufferSequence MB>
    capy::task<capy::io_result<std::size_t>>
    read_some(MB const& buffers)
    {
        // Zero-sized buffer completes immediately
        if(capy::buffer_size(buffers) == 0)
            co_return {{}, 0};

        // Main read loop - follows parser's read_some pattern
        for(;;)
        {
            // Parse any pending data
            system::error_code ec;
            pr_.parse(ec);

            // Check if we have body data available
            if(pr_.got_header())
            {
                // Check for available body data
                auto body_data = pr_.pull_body();
                if(capy::buffer_size(body_data) > 0)
                {
                    std::size_t n = capy::copy(buffers, body_data);
                    pr_.consume_body(n);
                    co_return {{}, n};
                }

                // No body data available
                if(pr_.is_complete())
                    co_return {capy::error::eof, 0};
            }

            // Need more data?
            if(ec == condition::need_more_input)
            {
                // Read from underlying stream
                auto mbs = pr_.prepare();
                auto [read_ec, bytes_read] = co_await stream_.read_some(mbs);

                if(read_ec)
                {
                    if(read_ec == capy::cond::eof)
                    {
                        pr_.commit_eof();
                    }
                    else
                    {
                        co_return {read_ec, 0};
                    }
                }
                else
                {
                    pr_.commit(bytes_read);
                }
                continue;
            }

            // Other errors
            if(ec && ec != error::end_of_message)
            {
                co_return {ec, 0};
            }
        }
    }
};

} // namespace http
} // namespace boost

#endif // BOOST_HTTP_BODY_READ_STREAM_HPP
