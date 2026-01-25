//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_BODY_WRITE_STREAM_HPP
#define BOOST_HTTP_BODY_WRITE_STREAM_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/serializer.hpp>

#include <boost/assert.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/capy/buffers/buffer_copy.hpp>
#include <boost/capy/concept/write_stream.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <utility>

namespace boost {
namespace http {

/** A stream adapter for writing HTTP message bodies.

    This class wraps an underlying @ref capy::WriteStream and an HTTP
    @ref serializer to provide a simple interface for writing message
    body data. The caller provides raw body bytes; the serializer
    automatically handles:

    @li HTTP headers (already written before body streaming begins)
    @li Chunked transfer-encoding (chunk framing added automatically)
    @li Content-Encoding compression (gzip, deflate, brotli if configured)
    @li Content-Length validation (if specified in headers)

    The class satisfies @ref capy::WriteStream, enabling composition
    with generic algorithms like @ref capy::write.

    @tparam Stream The underlying stream type satisfying
        @ref capy::WriteStream.

    @par Thread Safety
    Distinct objects: Safe.
    Shared objects: Unsafe.

    @par Example
    @code
    capy::task<void>
    send_response(capy::WriteStream auto& socket)
    {
        http::response res;
        res.set_chunked(true);  // Enable chunked encoding
        res.set(http::field::content_type, "text/plain");
        http::serializer sr(cfg, res);

        // Start streaming mode
        sr.start_stream();

        // Wrap socket + serializer for body writing
        http::body_write_stream bws(socket, sr);

        // Write body data - chunking handled automatically
        std::string_view body = "Hello, World!";
        auto [ec, n] = co_await bws.write_some(
            capy::make_buffer(body));
        if (ec)
            co_return;

        // Close signals end-of-body, flushes final chunk
        auto [ec2] = co_await bws.close();
    }
    @endcode

    @par Deferred Error Reporting
    If an error occurs after body data has been committed to the
    serializer, the operation reports success with the number of
    bytes consumed. The error is saved and reported on the next
    call to @ref write_some or @ref close. This ensures the caller
    knows exactly how many bytes were accepted.

    @see capy::WriteStream, serializer
*/
template<capy::WriteStream Stream>
class body_write_stream
{
    Stream& stream_;
    serializer& sr_;
    system::error_code saved_ec_;

public:
    /** Constructor.

        @param s The underlying stream to write serialized data to.
        @param sr The serializer performing HTTP framing.
            The serializer must be in streaming mode (after calling
            @ref serializer::start_stream).
    */
    explicit
    body_write_stream(
        Stream& s,
        serializer& sr) noexcept
        : stream_(s)
        , sr_(sr)
    {
    }

    /** Write body data to the message.

        Copies data from @p buffers into the serializer's internal
        buffer, then writes serialized output (with any encoding
        transformations) to the underlying stream.

        @tparam CB A type satisfying @ref capy::ConstBufferSequence.

        @param buffers The buffer sequence containing body data.

        @return A task yielding `(error_code, bytes_transferred)`.
            On success, `bytes_transferred` is the number of bytes
            consumed from @p buffers (may be less than total size).
            On error, `bytes_transferred` is 0 unless deferred error
            reporting applies.

        @par Example
        @code
        auto [ec, n] = co_await bws.write_some(
            capy::const_buffer(data, size));
        @endcode
    */
    auto
    write_some(
        capy::ConstBufferSequence auto const& buffers) ->
            capy::task<capy::io_result<std::size_t>>
    {
        BOOST_ASSERT(!sr_.is_done());

        // Check for saved error from previous call
        if(saved_ec_)
        {
            auto ec = saved_ec_;
            saved_ec_ = {};
            co_return {ec, 0};
        }

        // Zero-sized buffer completes immediately
        if(capy::buffer_empty(buffers))
            co_return {{}, 0};

        std::size_t bytes = 0;

        // Loop until at least one byte is consumed from input
        for(;;)
        {
            // Copy data from input buffers to serializer stream
            bytes = capy::buffer_copy(sr_.stream_prepare(), buffers);
            sr_.stream_commit(bytes);

            // Write serializer output to underlying stream
            auto cbs = sr_.prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                {
                    // Need to flush more data before we can accept input
                    // This shouldn't happen after commit, but handle it
                    if(bytes != 0)
                        break;
                    continue;
                }
                // Propagate other errors
                if(bytes != 0)
                {
                    saved_ec_ = cbs.error();
                    break;
                }
                co_return {cbs.error(), 0};
            }

            auto [ec, n] = co_await stream_.write_some(*cbs);
            sr_.consume(n);

            if(ec)
            {
                if(bytes != 0)
                {
                    saved_ec_ = ec;
                    break;
                }
                co_return {ec, 0};
            }

            if(bytes != 0)
                break;
        }

        co_return {{}, bytes};
    }

    /** Close the body stream and flush remaining data.

        Signals end-of-body to the serializer and writes any
        remaining buffered data to the underlying stream. For
        chunked encoding, this writes the final zero-length chunk.

        @return A task yielding `(error_code)`.

        @post The serializer's `is_done()` returns `true` on success.

        @par Example
        @code
        auto [ec] = co_await bws.close();
        if (!ec)
            // Message fully sent
        @endcode
    */
    capy::task<capy::io_result<>>
    close()
    {
        // Check for saved error from previous call
        if(saved_ec_)
        {
            auto ec = saved_ec_;
            saved_ec_ = {};
            co_return {ec};
        }

        // Signal end-of-body to serializer
        sr_.stream_close();

        // Flush all remaining serializer data
        while(!sr_.is_done())
        {
            auto cbs = sr_.prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                {
                    // Stream closed, no more data needed
                    continue;
                }
                co_return {cbs.error()};
            }

            if(capy::buffer_empty(*cbs))
            {
                sr_.consume(0);
                continue;
            }

            auto [ec, n] = co_await stream_.write_some(*cbs);
            sr_.consume(n);

            if(ec)
                co_return {ec};
        }

        co_return {{}};
    }
};

} // namespace http
} // namespace boost

#endif // BOOST_HTTP_BODY_WRITE_STREAM_HPP
