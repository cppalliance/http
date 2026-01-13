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

#include <boost/http/detail/config.hpp>
#include <boost/http/detail/workspace.hpp>
#include <boost/http/source.hpp>

#include <boost/capy/buffers/buffer_pair.hpp>
#include <boost/core/span.hpp>
#include <boost/capy/polystore_fwd.hpp>
#include <boost/system/result.hpp>

#include <type_traits>
#include <utility>

namespace boost {
namespace http {

// Forward declaration
class message_base;

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
    class stream;
    struct config;

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

    /** Constructor
        Default-constructed serializers do not reference any implementation;
        the only valid operations are destruction and assignment.
    */
    serializer() = default;

    /** Constructor.

        The states of `other` are transferred
        to the newly constructed object,
        which includes the allocated buffer.
        After construction, the only valid
        operations on the moved-from object
        are destruction and assignment.

        Buffer sequences previously obtained
        using @ref prepare or @ref stream::prepare
        remain valid.

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
        using @ref prepare or @ref stream::prepare
        remain valid.
        @par Complexity
        Constant.
        @param other The serializer to move from.
        @return A reference to this object.
    */
    BOOST_HTTP_DECL
    serializer&
    operator=(serializer&& other) noexcept;

    /** Constructor.

        Constructs a serializer that uses the @ref
        config parameters installed on the
        provided `ctx`.

        The serializer will attempt to allocate
        the required space on startup, with the
        amount depending on the @ref config
        parameters, and will not perform any
        further allocations, except for Brotli
        encoder instances, if enabled.

        Depending on which compression algorithms
        are enabled in the @ref config, the
        serializer will attempt to access the
        corresponding encoder services on the same
        `ctx`.

        @par Example
        @code
        serializer sr(ctx);
        @endcode

        @par Postconditions
        @code
        this->is_done() == true
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Calls to allocate may throw.

        @param ctx Context from which the
        serializer will access registered
        services. The caller is responsible for
        ensuring that the provided ctx remains
        valid for the lifetime of the serializer.

        @see
            @ref install_serializer_service,
            @ref config.
    */
    BOOST_HTTP_DECL
    explicit
    serializer(
        capy::polystore& ctx);

    /** Reset the serializer for a new message.

        Aborts any ongoing serialization and
        prepares the serializer to start
        serialization of a new message.
    */
    BOOST_HTTP_DECL
    void
    reset() noexcept;

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
        capy::const_buffer_sequence<ConstBufferSequence>
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
            capy::const_buffer_sequence<ConstBufferSequence>>::type
    >
    void
    start(
        message_base const& m,
        ConstBufferSequence&& buffers);

    /** Start serializing a message with a @em Source body

        Initializes the serializer with the HTTP start-line and headers from
        `m`, and constructs a `Source` object to provide the message body.

        Changing the contents of the message
        after calling this function and before
        @ref is_done returns `true` results in
        undefined behavior.

        The serializer destroys Source object when:
        @li `this->is_done() == true`
        @li An unrecoverable serialization error occurs
        @li The serializer is destroyed

        @par Example
        @code
        file f("example.zip", file_mode::scan);
        response.set_payload_size(f.size());
        serializer.start<file_source>(response, std::move(f));
        @endcode

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
        is_source<Source>::value == true
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

        @param args Arguments to be passed to the
        `Source` constructor.

        @return A reference to the constructed Source object.

        @see
            @ref source,
            @ref file_source,
            @ref message_base.
    */
    template<
        class Source,
        class... Args,
        class = typename std::enable_if<
            is_source<Source>::value>::type>
    Source&
    start(
        message_base const& m,
        Args&&... args);

    /** Prepare the serializer for a new message using a stream interface.

        Initializes the serializer with the HTTP
        start-line and headers from `m`, and returns
        a @ref stream object for writing the body
        data into the serializer's internal buffer.

        Once the serializer is destroyed, @ref reset
        is called, or @ref is_done returns true, the
        only valid operation on the stream is destruction.

        The stream allows inverted control flow: the
        caller supplies body data to the serializer’s
        internal buffer while reading from an external
        source.

        Changing the contents of the message
        after calling this function and before
        @ref is_done returns `true` results in
        undefined behavior.

        @par Example
        @code
        serializer::stream st = serializer.start_stream(response);
        do
        {
            if(st.is_open())
            {
                std::size_t n = source.read_some(st.prepare());

                if(ec == error::eof)
                    st.close();
                else
                    st.commit(n);
            }

            write_some(client, serializer);

        } while(!serializer.is_done());
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

        @return A @ref stream object for writing the body
        data into the serializer's internal buffer.

        @see
            @ref stream,
            @ref message_base.
     */
    BOOST_HTTP_DECL
    stream
    start_stream(
        message_base const& m);

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

        When the serializer is used through the
        @ref stream interface, the result may
        contain @ref error::need_data to indicate
        that additional input is required to
        produce output.

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

    BOOST_HTTP_DECL
    void
    start_source(
        message_base const&,
        source&);

    impl* impl_ = nullptr;
};

/** Serializer configuration settings.

    @see
        @ref install_serializer_service,
        @ref serializer.
*/
struct serializer::config
{
    /** Enable Brotli Content-Encoding.

        Requires `boost::capy::brotli::encode_service` to be
        installed, otherwise an exception is thrown.
    */
    bool apply_brotli_encoder = false;

    /** Enable Deflate Content-Encoding.

        Requires `boost::zlib::deflate_service` to be
        installed, otherwise an exception is thrown.
    */
    bool apply_deflate_encoder = false;

    /** Enable Gzip Content-Encoding.

        Requires `boost::zlib::deflate_service` to be
        installed, otherwise an exception is thrown.
    */
    bool apply_gzip_encoder = false;

    /** Brotli compression quality (0–11).

        Higher values yield better but slower compression.
    */
    std::uint32_t brotli_comp_quality = 5;

    /** Brotli compression window size (10–24).

        Larger windows improve compression but increase
        memory usage.
    */
    std::uint32_t brotli_comp_window = 18;

    /** Zlib compression level (0–9).

        0 = no compression, 1 = fastest, 9 = best
        compression.
    */
    int zlib_comp_level = 6;

    /** Zlib window bits (9–15).

        Controls the history buffer size. Larger values
        improve compression but use more memory.
    */
    int zlib_window_bits = 15;

    /** Zlib memory level (1–9).

        Higher values use more memory, but offer faster
        and more efficient compression.
    */
    int zlib_mem_level = 8;

    /** Minimum buffer size for payloads (must be > 0). */
    std::size_t payload_buffer = 8192;

    /** Reserved space for type-erasure storage.

        Used for:
        @li User-defined @ref source objects.
        @li User-defined ConstBufferSequence instances.
    */
    std::size_t max_type_erase = 1024;
};

/** Install the serializer service.

    @par Example
    @code
    // default configuration settings
    install_serializer_service(ctx, {});

    serializer sr(ctx);
    @endcode

    @par Exception Safety
    Strong guarantee.

    @throw std::invalid_argument If the service is
    already installed on the context.

    @param ctx Reference to the context on which
    the service should be installed.

    @param cfg Configuration settings for the
    serializer.

    @see
        @ref serializer::config,
        @ref serializer.
*/
BOOST_HTTP_DECL
void
install_serializer_service(
    capy::polystore& ctx,
    serializer::config const& cfg);

//------------------------------------------------

/** Used for streaming body data during serialization.

    Provides an interface for supplying serialized
    body content from an external source. This
    object is returned by @ref
    serializer::start_stream and enables
    incremental writing of the message body into
    the serializer's internal buffer.

    The stream supports an inverted control flow
    model, where the caller pushes body data as
    needed.

    Valid operations depend on the state of the
    serializer. Once the serializer is destroyed,
    reset, or completes, the stream becomes
    invalid and must only be destroyed.

    @see
        @ref serializer::start_stream
*/
class serializer::stream
{
public:
    /** The type used to represent a sequence
        of mutable buffers.
    */
    using mutable_buffers_type =
        capy::mutable_buffer_pair;

    /** Constructor.

        A default-constructed stream is
        considered closed.

        @par Postconditions
        @code
        this->is_open() == false
        @endcode
    */
    stream() noexcept = default;

    /** Constructor.

        After construction, the moved-from
        object is as if default-constructed.

        @par Postconditions
        @code
        other->is_open() == false
        @endcode

        @param other The object to move from.
    */
    stream(stream&& other) noexcept
        : impl_(other.impl_)
    {
        other.impl_ = nullptr;
    }

    /** Move assignment.

        After assignment, the moved-from
        object is as if default-constructed.

        @par Postconditions
        @code
        other->is_open() == false
        @endcode

        @param other The object to assign from.
        @return A reference to this object.
    */
    stream&
    operator=(stream&& other) noexcept
    {
        std::swap(impl_, other.impl_);
        return *this;
    }

    /** Return true if the stream is open.
    */
    bool
    is_open() const noexcept
    {
        return impl_ != nullptr;
    }

    /** Return the available capacity.

        @par Preconditions
        @code
        this->is_open() == true
        @endcode

        @par Exception Safety
        Strong guarantee.

        @throw std::logic_error
        `this->is_open() == false`.
    */
    BOOST_HTTP_DECL
    std::size_t
    capacity() const;

    /** Prepare a buffer for writing.

        Retuns a mutable buffer sequence representing
        the writable bytes. Use @ref commit to make the
        written data available to the serializer.

        All buffer sequences previously obtained
        using @ref prepare are invalidated.

        @par Preconditions
        @code
        this->is_open() == true && n <= this->capacity()
        @endcode

        @par Exception Safety
        Strong guarantee.

        @return An instance of @ref mutable_buffers_type
        the underlying memory is owned by the serializer.

        @throw std::logic_error
        `this->is_open() == false`

        @see
            @ref commit,
            @ref capacity.
    */
    BOOST_HTTP_DECL
    mutable_buffers_type
    prepare();

    /** Commit data to the serializer.

        Makes `n` bytes available to the serializer.

        All buffer sequences previously obtained
        using @ref prepare are invalidated.

        @par Preconditions
        @code
        this->is_open() == true && n <= this->capacity()
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown on invalid input.

        @param n The number of bytes to append.

        @throw std::invalid_argument
        `n > this->capacity()`

        @throw std::logic_error
        `this->is_open() == false`

        @see
            @ref prepare,
            @ref capacity.
    */
    BOOST_HTTP_DECL
    void
    commit(std::size_t n);

    /** Close the stream if open.

        Closes the stream and
        notifies the serializer that the
        message body has ended.

        If the stream is already closed this
        call has no effect.

        @par Postconditions
        @code
        this->is_open() == false
        @endcode
    */
    BOOST_HTTP_DECL
    void
    close() noexcept;

    /** Destructor.

        Closes the stream if open.
    */
    ~stream()
    {
        close();
    }

private:
    friend class serializer;

    explicit
    stream(serializer::impl* impl) noexcept
        : impl_(impl)
    {
    }

    serializer::impl* impl_ = nullptr;
};

} // http
} // boost

#include <boost/http/impl/serializer.hpp>

#endif
