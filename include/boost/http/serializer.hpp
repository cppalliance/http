//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERIALIZER_HPP
#define BOOST_HTTP_SERIALIZER_HPP

#include <boost/http/config.hpp>
#include <boost/http/detail/workspace.hpp>
#include <boost/http/error.hpp>

#include <boost/capy/buffers.hpp>
#include <boost/http/concept/buffer_sink.hpp>
#include <boost/capy/concept/write_stream.hpp>
#include <boost/capy/io_task.hpp>
#include <boost/core/span.hpp>
#include <boost/system/result.hpp>

#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

namespace boost {
namespace http {

// Forward declaration
class message_base;

//------------------------------------------------

/** A serializer for HTTP/1 messages.

    Transforms one or more HTTP/1 messages into bytes for
    transmission. Each message consists of a required header
    followed by an optional body.

    Use @ref set_message to associate a message, then choose
    a body mode:

    @li @ref start — empty body (header only)
    @li @ref start_writes — body via internal buffer
        (BufferSink path)
    @li @ref start_buffers — body via caller-owned buffers
        (WriteSink path)

    Alternatively, obtain a @ref sink via @ref sink_for and
    let it start the serializer lazily on first use.

    The caller must ensure that the associated message is not
    changed or destroyed until @ref is_done returns true,
    @ref reset is called, or the serializer is destroyed.

    @par Example
    @code
    http::serializer sr(cfg);
    http::response res;
    res.set_payload_size(5);
    sr.set_message(res);

    auto sink = sr.sink_for(socket);
    co_await sink.write_eof(
        capy::make_buffer(std::string_view("hello")));
    @endcode

    @see @ref sink, @ref set_message.
*/
class serializer
{
public:
    template<capy::WriteStream Stream>
    class sink;

    /** The type used to represent a sequence
        of mutable buffers for streaming.
    */
    using mutable_buffers_type =
        std::array<capy::mutable_buffer, 2>;

    /** The type used to represent a sequence of
        constant buffers that refers to the output
        area.
    */
    using const_buffers_type =
        boost::span<capy::const_buffer const>;

    /** Destructor
    */
    BOOST_HTTP_DECL
    ~serializer();

    /** Default constructor.

        Constructs a serializer with no allocated state.
        The serializer must be assigned from a valid
        serializer before use.

        @par Postconditions
        The serializer has no allocated state.
    */
    serializer() = default;

    /** Constructor.

        Constructs a serializer with the provided configuration.

        @par Postconditions
        @code
        this->is_done() == true
        @endcode

        @param cfg Shared pointer to serializer configuration.

        @see @ref make_serializer_config, @ref serializer_config.
    */
    BOOST_HTTP_DECL
    explicit
    serializer(
        std::shared_ptr<serializer_config_impl const> cfg);

    /** Constructor.

        The states of `other` are transferred
        to the newly constructed object,
        which includes the allocated buffer.
        After construction, the only valid
        operations on the moved-from object
        are destruction and assignment.

        Buffer sequences previously obtained
        using @ref prepare remain valid.

        @par Postconditions
        @code
        other.is_done() == true
        @endcode

        @par Complexity
        Constant.

        @param other The serializer to move from.
    */
    BOOST_HTTP_DECL
    serializer(
        serializer&& other) noexcept;

    /** Assignment.
        The states of `other` are transferred
        to this object, which includes the
        allocated buffer. After assignment,
        the only valid operations on the
        moved-from object are destruction and
        assignment.
        Buffer sequences previously obtained
        using @ref prepare remain valid.
        @par Complexity
        Constant.
        @param other The serializer to move from.
        @return A reference to this object.
    */
    BOOST_HTTP_DECL
    serializer&
    operator=(serializer&& other) noexcept;

    /** Reset the serializer for a new message.

        Aborts any ongoing serialization and
        prepares the serializer to start
        serialization of a new message.
    */
    BOOST_HTTP_DECL
    void
    reset() noexcept;

    /** Set the message to serialize.

        Associates a message with the serializer for subsequent
        streaming operations. The message is not copied; the caller
        must ensure it remains valid until serialization completes.

        @param m The message to associate.
    */
    BOOST_HTTP_DECL
    void
    set_message(message_base const& m) noexcept;

    /** Start serializing the associated message with an empty body.

        The message must be set beforehand using @ref set_message.
        Use the prepare/consume loop to pull output bytes.

        @par Preconditions
        A message was associated via @ref set_message.

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error if no message is associated or
        `this->is_done() == false`.

        @throw std::length_error if there is insufficient internal buffer
        space to start the operation.

        @see @ref set_message, @ref prepare, @ref consume.
    */
    void
    BOOST_HTTP_DECL
    start();

    /** Start streaming the associated message.

        Low-level entry point equivalent to @ref start_writes.
        Prefer using a @ref sink which starts lazily.

        @par Preconditions
        A message was associated via @ref set_message.

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error if no message is associated or
        `this->is_done() == false`.

        @throw std::length_error if there is insufficient internal buffer
        space to start the operation.

        @see @ref start_writes, @ref sink.
    */
    BOOST_HTTP_DECL
    void
    start_stream();

    /** Start the serializer in write mode.

        Prepares the serializer for write-mode streaming
        using the message previously set via @ref set_message.
        In this mode, the workspace is split into an input
        buffer and an output buffer. Use @ref stream_prepare,
        @ref stream_commit, and @ref stream_close to write
        body data, or use the sink's BufferSink interface.

        @par Preconditions
        A message was associated via @ref set_message.
        @code
        this->is_done() == true
        @endcode

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error if no message is associated.

        @throw std::length_error if there is insufficient internal buffer
        space to start the operation.

        @see @ref set_message, @ref sink.
    */
    BOOST_HTTP_DECL
    void
    start_writes();

    /** Start the serializer in buffer mode.

        Prepares the serializer for buffer-mode streaming
        using the message previously set via @ref set_message.
        In this mode, the entire workspace is used for output
        buffering. The caller provides body data through the
        sink's WriteSink methods (write, write_eof), passing
        their own buffers directly.

        @par Preconditions
        A message was associated via @ref set_message.
        @code
        this->is_done() == true
        @endcode

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error if no message is associated.

        @throw std::length_error if there is insufficient internal buffer
        space to start the operation.

        @see @ref set_message, @ref sink.
    */
    BOOST_HTTP_DECL
    void
    start_buffers();

    /** Create a sink for writing body data.

        Returns a lightweight @ref sink handle that writes
        serialized body data to the provided stream. The sink
        starts the serializer lazily on first use, so neither
        @ref start_writes nor @ref start_buffers need to be
        called beforehand.

        The sink can be created once and reused across multiple
        messages. The serializer must outlive the sink.

        @par Example
        @code
        http::serializer sr(cfg);
        auto sink = sr.sink_for(socket);

        http::response res;
        res.set_payload_size(5);
        sr.set_message(res);
        co_await sink.write_eof(
            capy::make_buffer(std::string_view("hello")));
        @endcode

        @tparam Stream The output stream type satisfying
            @ref capy::WriteStream.

        @param ws The output stream to write serialized data to.

        @return A @ref sink object for writing body data.

        @see @ref sink, @ref set_message.
    */
    template<capy::WriteStream Stream>
    sink<Stream>
    sink_for(Stream& ws) noexcept;

    /** Return the output area.

        This function serializes some or all of
        the message and returns the corresponding
        output buffers. Afterward, a call to @ref
        consume is required to report the number
        of bytes used, if any.

        If the message includes an
        `Expect: 100-continue` header and the
        header section of the message has been
        consumed, the returned result will contain
        @ref error::expect_100_continue to
        indicate that the header part of the
        message is complete. The next call to @ref
        prepare will produce output.

        When the serializer is in streaming mode,
        the result may contain @ref error::need_data
        to indicate that additional input is required
        to produce output.

        @par Preconditions
        @code
        this->is_done() == false
        @endcode
        No unrecoverable error reported from previous calls.

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error
        `this->is_done() == true`.

        @return A result containing @ref
        const_buffers_type that represents the
        output area or an error if any occurred.

        @see
            @ref consume,
            @ref is_done,
            @ref const_buffers_type.
    */
    BOOST_HTTP_DECL
    auto
    prepare() ->
        system::result<
            const_buffers_type>;

    /** Consume bytes from the output area.

        This function should be called after one
        or more bytes contained in the buffers
        provided in the prior call to @ref prepare
        have been used.

        After a call to @ref consume, callers
        should check the return value of @ref
        is_done to determine if the entire message
        has been serialized.

        @par Preconditions
        @code
        this->is_done() == false
        @endcode

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error
        `this->is_done() == true`.

        @param n The number of bytes to consume.
        If `n` is greater than the size of the
        buffer returned from @ref prepared the
        entire output sequence is consumed and no
        error is issued.

        @see
            @ref prepare,
            @ref is_done,
            @ref const_buffers_type.
    */
    BOOST_HTTP_DECL
    void
    consume(std::size_t n);

    /** Return true if serialization is complete.
    */
    BOOST_HTTP_DECL
    bool
    is_done() const noexcept;

    /** Return true if serialization has not yet started.
    */
    BOOST_HTTP_DECL
    bool
    is_start() const noexcept;

    /** Return the available capacity for streaming.

        Returns the number of bytes that can be written
        to the serializer's internal buffer.

        @par Preconditions
        The serializer is in streaming mode (after calling
        @ref start_stream).

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error if not in streaming mode.
    */
    BOOST_HTTP_DECL
    std::size_t
    stream_capacity() const;

    /** Prepare a buffer for writing stream data.

        Returns a mutable buffer sequence representing
        the writable bytes. Use @ref stream_commit to make the
        written data available to the serializer.

        All buffer sequences previously obtained
        using @ref stream_prepare are invalidated.

        @par Preconditions
        The serializer is in streaming mode.

        @par Exception Safety
        Strong guarantee.

        @return An instance of @ref mutable_buffers_type.
            The underlying memory is owned by the serializer.

        @throw std::logic_error if not in streaming mode.

        @see
            @ref stream_commit,
            @ref stream_capacity.
    */
    BOOST_HTTP_DECL
    mutable_buffers_type
    stream_prepare();

    /** Commit data to the serializer stream.

        Makes `n` bytes available to the serializer.

        All buffer sequences previously obtained
        using @ref stream_prepare are invalidated.

        @par Preconditions
        The serializer is in streaming mode and
        `n <= stream_capacity()`.

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown on invalid input.

        @param n The number of bytes to commit.

        @throw std::invalid_argument if `n > stream_capacity()`.

        @throw std::logic_error if not in streaming mode.

        @see
            @ref stream_prepare,
            @ref stream_capacity.
    */
    BOOST_HTTP_DECL
    void
    stream_commit(std::size_t n);

    /** Close the stream.

        Notifies the serializer that the message body
        has ended. After calling this function, no more
        data can be written to the stream.

        @par Preconditions
        The serializer is in streaming mode.

        @par Postconditions
        The stream is closed.
    */
    BOOST_HTTP_DECL
    void
    stream_close() noexcept;

private:
    class impl;

    BOOST_HTTP_DECL
    detail::workspace&
    ws();

    impl* impl_ = nullptr;
};

//------------------------------------------------

/** A sink adapter for writing HTTP message bodies.

    Wraps a @ref serializer and a @ref capy::WriteStream to
    provide two interfaces for body writing:

    @li **BufferSink** (@ref prepare / @ref commit /
        @ref commit_eof) — write directly into the serializer's
        internal buffer (zero-copy). Triggers @ref start_writes
        lazily.
    @li **WriteSink** (@ref write / @ref write_eof) — pass
        caller-owned buffers; the sink copies data through the
        serializer. Triggers @ref start_buffers lazily.

    Both interfaces handle chunked framing, compression, and
    Content-Length validation automatically.

    The sink is a lightweight handle that can be created once
    and reused across multiple messages. The serializer and
    stream must outlive the sink.

    @tparam Stream The underlying stream type satisfying
        @ref capy::WriteStream.

    @par Thread Safety
    Distinct objects: Safe.
    Shared objects: Unsafe.

    @par Example
    @code
    capy::task<>
    send_response(capy::WriteStream auto& socket)
    {
        http::serializer sr(cfg);
        auto sink = sr.sink_for(socket);

        http::response res;
        res.set_payload_size(5);
        sr.set_message(res);

        // WriteSink: pass your own buffer
        co_await sink.write_eof(
            capy::make_buffer(std::string_view("hello")));
    }
    @endcode

    @see @ref http::BufferSink, @ref http::any_buffer_sink,
        @ref serializer.
*/
template<capy::WriteStream Stream>
class serializer::sink
{
    Stream* stream_ = nullptr;
    serializer* sr_ = nullptr;

public:
    /** Constructor.

        A default-constructed sink is in an empty state.
    */
    sink() noexcept = default;

    /** Constructor.

        @param stream The underlying stream to write serialized data to.
        @param sr The serializer performing HTTP framing.
    */
    sink(
        Stream& stream,
        serializer& sr) noexcept
        : stream_(&stream)
        , sr_(&sr)
    {
    }

    /** Prepare writable buffers.

        Fills the provided span with mutable buffer descriptors
        pointing to the serializer's internal storage. This
        operation is synchronous. Lazily starts the serializer
        in write mode if not already started.

        @param dest Span of mutable_buffer to fill.

        @return A span of filled buffers.
    */
    std::span<capy::mutable_buffer>
    prepare(std::span<capy::mutable_buffer> dest)
    {
        if(sr_->is_start())
            sr_->start_writes();
        auto bufs = sr_->stream_prepare();
        std::size_t count = 0;
        for(auto const& b : bufs)
        {
            if(count >= dest.size() || b.size() == 0)
                break;
            dest[count++] = b;
        }
        return dest.first(count);
    }

    /** Commit bytes written to the prepared buffers.

        Commits `n` bytes written to the buffers returned by the
        most recent call to @ref prepare. The operation flushes
        serialized output to the underlying stream.

        @param n The number of bytes to commit.

        @return An awaitable yielding `(error_code)`.
    */
    auto
    commit(std::size_t n)
        -> capy::io_task<>
    {
        if(sr_->is_start())
            sr_->start_writes();
        sr_->stream_commit(n);

        while(!sr_->is_done())
        {
            auto cbs = sr_->prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                    break;
                co_return {std::error_code(cbs.error())};
            }

            if(capy::buffer_empty(*cbs))
            {
                // advance state machine
                sr_->consume(0);
                continue;
            }

            auto [ec, written] = co_await stream_->write_some(*cbs);
            sr_->consume(written);

            if(ec)
                co_return {ec};
        }

        co_return {};
    }

    /** Commit final bytes and signal end-of-stream.

        Commits `n` bytes written to the buffers returned by the
        most recent call to @ref prepare and closes the body stream,
        flushing any remaining serializer output to the underlying
        stream. For chunked encoding, this writes the final
        zero-length chunk.

        @param n The number of bytes to commit.

        @return An awaitable yielding `(error_code)`.

        @post The serializer's `is_done()` returns `true` on success.
    */
    auto
    commit_eof(std::size_t n)
        -> capy::io_task<>
    {
        if(sr_->is_start())
            sr_->start_writes();
        sr_->stream_commit(n);
        sr_->stream_close();

        while(!sr_->is_done())
        {
            auto cbs = sr_->prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                    continue;
                co_return {std::error_code(cbs.error())};
            }

            if(capy::buffer_empty(*cbs))
            {
                // advance state machine
                sr_->consume(0);
                continue;
            }

            auto [ec, written] = co_await stream_->write_some(*cbs);
            sr_->consume(written);

            if(ec)
                co_return {ec};
        }

        co_return {};
    }

    /** Write body data from caller-owned buffers.

        Lazily starts the serializer in buffer mode if not
        already started. Writes all data from the provided
        buffers through the serializer to the underlying stream.

        @param buffers The buffer sequence containing body data.

        @return An awaitable yielding `(error_code, std::size_t)`.
        The size_t is the total number of body bytes written.
    */
    template<class ConstBufferSequence>
    auto
    write(ConstBufferSequence const& buffers)
        -> capy::io_task<std::size_t>
    {
        if(sr_->is_start())
            sr_->start_buffers();

        // Drain header first
        while(!sr_->is_done())
        {
            auto cbs = sr_->prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                    break;
                co_return {cbs.error(), 0};
            }

            if(capy::buffer_empty(*cbs))
            {
                // advance state machine
                sr_->consume(0);
                continue;
            }

            auto [ec, written] = co_await stream_->write_some(*cbs);
            sr_->consume(written);

            if(ec)
                co_return {ec, 0};
        }

        // Write body data through stream_prepare/commit
        std::size_t total = 0;
        for(auto it = capy::begin(buffers);
            it != capy::end(buffers); ++it)
        {
            capy::const_buffer src = *it;
            while(src.size() != 0)
            {
                auto mbp = sr_->stream_prepare();
                std::size_t copied = 0;
                for(auto const& mb : mbp)
                {
                    auto chunk = (std::min)(
                        mb.size(), src.size());
                    if(chunk == 0)
                        break;
                    std::memcpy(mb.data(),
                        src.data(), chunk);
                    src += chunk;
                    copied += chunk;
                }
                sr_->stream_commit(copied);
                total += copied;

                // Drain output
                while(!sr_->is_done())
                {
                    auto cbs = sr_->prepare();
                    if(cbs.has_error())
                    {
                        if(cbs.error() == error::need_data)
                            break;
                        co_return {cbs.error(), total};
                    }

                    if(capy::buffer_empty(*cbs))
                    {
                        // advance state machine
                        sr_->consume(0);
                        continue;
                    }

                    auto [ec, written] =
                        co_await stream_->write_some(*cbs);
                    sr_->consume(written);

                    if(ec)
                        co_return {ec, total};
                }
            }
        }

        co_return {std::error_code(), total};
    }

    /** Write final body data and signal end-of-stream.

        Lazily starts the serializer in buffer mode if not
        already started. Writes all data from the provided
        buffers and then closes the body stream, flushing
        any remaining output to the underlying stream.

        @param buffers The buffer sequence containing final body data.

        @return An awaitable yielding `(error_code, std::size_t)`.
        The size_t is the total number of body bytes written.

        @post The serializer's `is_done()` returns `true` on success.
    */
    template<class ConstBufferSequence>
    auto
    write_eof(ConstBufferSequence const& buffers)
        -> capy::io_task<std::size_t>
    {
        auto [ec, n] = co_await write(buffers);
        if(ec)
            co_return {ec, n};

        sr_->stream_close();

        while(!sr_->is_done())
        {
            auto cbs = sr_->prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                    continue;
                co_return {cbs.error(), n};
            }

            if(capy::buffer_empty(*cbs))
            {
                // advance state machine
                sr_->consume(0);
                continue;
            }

            auto [ec2, written] = co_await stream_->write_some(*cbs);
            sr_->consume(written);

            if(ec2)
                co_return {ec2, n};
        }

        co_return {std::error_code(), n};
    }

    /** Signal end-of-stream with no additional data.

        Lazily starts the serializer in buffer mode if not
        already started. Closes the body stream and flushes
        any remaining output to the underlying stream.

        @return An awaitable yielding `(error_code)`.

        @post The serializer's `is_done()` returns `true` on success.
    */
    auto
    write_eof()
        -> capy::io_task<>
    {
        if(sr_->is_start())
            sr_->start_buffers();

        sr_->stream_close();

        while(!sr_->is_done())
        {
            auto cbs = sr_->prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                    continue;
                co_return {std::error_code(cbs.error())};
            }

            if(capy::buffer_empty(*cbs))
            {
                // advance state machine
                sr_->consume(0);
                continue;
            }

            auto [ec, written] = co_await stream_->write_some(*cbs);
            sr_->consume(written);

            if(ec)
                co_return {ec};
        }

        co_return {};
    }
};

//------------------------------------------------

template<capy::WriteStream Stream>
serializer::sink<Stream>
serializer::sink_for(Stream& ws) noexcept
{
    return sink<Stream>(ws, *this);
}

} // http
} // boost

#endif
