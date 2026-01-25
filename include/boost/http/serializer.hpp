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
#include <boost/capy/buffers/buffer_copy.hpp>
#include <boost/capy/buffers/buffer_pair.hpp>
#include <boost/capy/concept/write_stream.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/core/span.hpp>
#include <boost/system/result.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace boost {
namespace http {

// Forward declaration
class message_base;

//------------------------------------------------

/** A serializer for HTTP/1 messages

    This is used to serialize one or more complete
    HTTP/1 messages. Each message consists of a
    required header followed by an optional body.

    Objects of this type operate using an "input area" and an
    "output area". Callers provide data to the input area
    using one of the @ref start or @ref start_stream member
    functions. After input is provided, serialized data
    becomes available in the serializer's output area in the
    form of a constant buffer sequence.

    Callers alternate between filling the input area and
    consuming the output area until all the input has been
    provided and all the output data has been consumed, or
    an error occurs.

    After calling @ref start, the caller must ensure that the
    contents of the associated message are not changed or
    destroyed until @ref is_done returns true, @ref reset is
    called, or the serializer is destroyed, otherwise the
    behavior is undefined.
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
        capy::mutable_buffer_pair;

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

    /** Constructor with associated message.

        Constructs a serializer with the provided configuration
        and associates a message for the serializer's lifetime.
        The message must remain valid until the serializer is
        destroyed.

        @par Postconditions
        @code
        this->is_done() == true
        @endcode

        @param cfg Shared pointer to serializer configuration.

        @param m The message to associate.

        @see @ref make_serializer_config, @ref serializer_config.
    */
    BOOST_HTTP_DECL
    serializer(
        std::shared_ptr<serializer_config_impl const> cfg,
        message_base const& m);

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

    /** Start serializing a message with an empty body

        This function prepares the serializer to create a message which
        has an empty body.
        Ownership of the specified message is not transferred; the caller is
        responsible for ensuring the lifetime of the object extends until the
        serializer is done.

        @par Preconditions
        @code
        this->is_done() == true
        @endcode

        @par Postconditions
        @code
        this->is_done() == false
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown if there is insufficient internal buffer space
        to start the operation.

        @throw std::logic_error `this->is_done() == true`.

        @throw std::length_error if there is insufficient internal buffer
        space to start the operation.

        @param m The request or response headers to serialize.

        @see
            @ref message_base.
    */
    void
    BOOST_HTTP_DECL
    start(message_base const& m);

    /** Start serializing the associated message with an empty body.

        Uses the message associated at construction time.

        @par Preconditions
        A message was associated at construction.
        @code
        this->is_done() == true
        @endcode

        @par Postconditions
        @code
        this->is_done() == false
        @endcode

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error if no message is associated or
        `this->is_done() == false`.

        @throw std::length_error if there is insufficient internal buffer
        space to start the operation.
    */
    void
    BOOST_HTTP_DECL
    start();

    /** Start serializing a message with a buffer sequence body

        Initializes the serializer with the HTTP start-line and headers from `m`,
        and the provided `buffers` for reading the message body from.

        Changing the contents of the message after calling this function and
        before @ref is_done returns `true` results in undefined behavior.

        At least one copy of the specified buffer sequence is maintained until
        the serializer is done, gets reset, or ios destroyed, after which all
        of its copies are destroyed. Ownership of the underlying memory is not
        transferred; the caller must ensure the memory remains valid until the
        serializer’s copies are destroyed.

        @par Preconditions
        @code
        this->is_done() == true
        @endcode

        @par Postconditions
        @code
        this->is_done() == false
        @endcode

        @par Constraints
        @code
        capy::ConstBufferSequence<ConstBufferSequence>
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown if there is insufficient internal buffer space
        to start the operation.

        @throw std::logic_error `this->is_done() == true`.

        @throw std::length_error If there is insufficient internal buffer
        space to start the operation.

        @param m The message to read the HTTP start-line and headers from.

        @param buffers A buffer sequence containing the message body.

        containing the message body data. While
        the buffers object is copied, ownership of
        the underlying memory remains with the
        caller, who must ensure it stays valid
        until @ref is_done returns `true`.

        @see
            @ref message_base.
    */
    template<
        class ConstBufferSequence,
        class = typename std::enable_if<
            capy::ConstBufferSequence<ConstBufferSequence>>::type
    >
    void
    start(
        message_base const& m,
        ConstBufferSequence&& buffers);

    /** Prepare the serializer for streaming body data.

        Initializes the serializer with the HTTP
        start-line and headers from `m` for streaming
        mode. After calling this function, use
        @ref stream_prepare, @ref stream_commit, and
        @ref stream_close to write body data.

        Changing the contents of the message
        after calling this function and before
        @ref is_done returns `true` results in
        undefined behavior.

        @par Example
        @code
        auto sink = sr.sink_for(socket);
        sr.start_stream(response);
        co_await sink.write(capy::make_buffer("Hello"));
        co_await sink.write_eof();
        @endcode

        @par Preconditions
        @code
        this->is_done() == true
        @endcode

        @par Postconditions
        @code
        this->is_done() == false
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown if there is insufficient
        internal buffer space to start the
        operation.

        @throw std::length_error if there is
        insufficient internal buffer space to
        start the operation.

        @param m The message to read the HTTP
        start-line and headers from.

        @see
            @ref stream_prepare,
            @ref stream_commit,
            @ref stream_close,
            @ref message_base.
     */
    BOOST_HTTP_DECL
    void
    start_stream(
        message_base const& m);

    /** Start streaming the associated message.

        Uses the message associated at construction time.

        @par Preconditions
        A message was associated at construction.
        @code
        this->is_done() == true
        @endcode

        @par Postconditions
        @code
        this->is_done() == false
        @endcode

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error if no message is associated or
        `this->is_done() == false`.

        @throw std::length_error if there is insufficient internal buffer
        space to start the operation.

        @see
            @ref stream_prepare,
            @ref stream_commit,
            @ref stream_close.
    */
    BOOST_HTTP_DECL
    void
    start_stream();

    /** Get a sink wrapper for writing body data.

        Returns a @ref sink object that can be used to write body
        data to the provided stream. This function does not call
        @ref start_stream. The caller must call @ref start_stream
        before using the sink.

        This allows the sink to be obtained early (e.g., at
        construction time) and stored, with streaming started
        later when the message is ready.

        @par Example
        @code
        http::serializer sr(cfg, res);
        auto sink = sr.sink_for(socket);
        // ... later ...
        sr.start_stream();  // Configure for streaming
        co_await sink.write(capy::make_buffer("Hello"));
        co_await sink.write_eof();
        @endcode

        @tparam Stream The output stream type satisfying
            @ref capy::WriteStream.

        @param ws The output stream to write serialized data to.

        @return A @ref sink object for writing body data.

        @see @ref sink, @ref start_stream.
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

        If a @ref source object is in use and a
        call to @ref source::read returns an
        error, the serializer enters a faulted
        state and propagates the error to the
        caller. This faulted state can only be
        cleared by calling @ref reset. This
        ensures the caller is explicitly aware
        that the previous message was truncated
        and that the stream must be terminated.

        @par Preconditions
        @code
        this->is_done() == false
        @endcode
        No unrecoverable error reported from previous calls.

        @par Exception Safety
        Strong guarantee.
        Calls to @ref source::read may throw if in use.

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
    class cbs_gen;
    template<class>
    class cbs_gen_impl;

    BOOST_HTTP_DECL
    detail::workspace&
    ws();

    BOOST_HTTP_DECL
    void
    start_init(
        message_base const&);

    BOOST_HTTP_DECL
    void
    start_buffers(
        message_base const&,
        cbs_gen&);

    impl* impl_ = nullptr;
};

//------------------------------------------------

/** A sink adapter for writing HTTP message bodies.

    This class wraps an underlying @ref capy::WriteStream and a
    @ref serializer to provide a @ref capy::WriteSink interface
    for writing message body data. The caller provides raw body
    bytes; the serializer automatically handles:

    @li Chunked transfer-encoding (chunk framing added automatically)
    @li Content-Encoding compression (gzip, deflate, brotli if configured)
    @li Content-Length validation (if specified in headers)

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
        res.set_chunked(true);
        http::serializer sr(cfg, res);

        auto sink = sr.sink_for(socket);
        sr.start_stream();
        co_await sink.write(capy::make_buffer("Hello"));
        co_await sink.write_eof();
    }
    @endcode

    @see capy::WriteSink, serializer
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

    /** Write body data.

        Copies data from @p buffers into the serializer's internal
        buffer, serializes it, and writes the output to the
        associated stream.

        @param buffers Buffer sequence containing body data.

        @return An awaitable yielding `(error_code, std::size_t)`.
            On success, all bytes from @p buffers have been consumed.
    */
    template<capy::ConstBufferSequence CB>
    auto
    write(CB buffers)
        -> capy::task<capy::io_result<std::size_t>>
    {
        return write(buffers, false);
    }

    /** Write body data with optional end-of-stream.

        Copies data from @p buffers into the serializer's internal
        buffer, serializes it, and writes the output to the
        associated stream. If @p eof is true, signals end-of-body
        after writing.

        @param buffers Buffer sequence containing body data.
        @param eof If true, signals end-of-body after writing.

        @return An awaitable yielding `(error_code, std::size_t)`.
            On success, all bytes from @p buffers have been consumed.
    */
    template<capy::ConstBufferSequence CB>
    auto
    write(CB buffers, bool eof)
        -> capy::task<capy::io_result<std::size_t>>
    {
        std::size_t bytes = capy::buffer_copy(sr_->stream_prepare(), buffers);
        sr_->stream_commit(bytes);

        if(eof)
            sr_->stream_close();

        while(!sr_->is_done())
        {
            auto cbs = sr_->prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data && !eof)
                    break;
                if(cbs.error() == error::need_data)
                    continue;
                co_return {cbs.error(), 0};
            }

            if(capy::buffer_empty(*cbs))
            {
                sr_->consume(0);
                continue;
            }

            auto [ec, n] = co_await stream_->write_some(*cbs);
            sr_->consume(n);

            if(ec.failed())
                co_return {ec, bytes};
        }

        co_return {{}, bytes};
    }

    /** Signal end of body data.

        Closes the body stream and flushes any remaining serializer
        output to the underlying stream. For chunked encoding, this
        writes the final zero-length chunk.

        @return An awaitable yielding `(error_code)`.

        @post The serializer's `is_done()` returns `true` on success.
    */
    auto
    write_eof()
        -> capy::task<capy::io_result<>>
    {
        sr_->stream_close();

        while(!sr_->is_done())
        {
            auto cbs = sr_->prepare();
            if(cbs.has_error())
            {
                if(cbs.error() == error::need_data)
                    continue;
                co_return {cbs.error()};
            }

            if(capy::buffer_empty(*cbs))
            {
                sr_->consume(0);
                continue;
            }

            auto [ec, n] = co_await stream_->write_some(*cbs);
            sr_->consume(n);

            if(ec.failed())
                co_return {ec};
        }

        co_return {{}};
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

#include <boost/http/impl/serializer.hpp>

#endif
