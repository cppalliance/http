//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_PARSER_HPP
#define BOOST_HTTP_PROTO_PARSER_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/parser_config.hpp>
#include <boost/http_proto/detail/header.hpp>
#include <boost/http_proto/detail/type_traits.hpp>
#include <boost/http_proto/detail/workspace.hpp>
#include <boost/http_proto/sink.hpp>

#include <boost/buffers/dynamic_buffer.hpp>
#include <boost/buffers/buffer_pair.hpp>
#include <boost/core/span.hpp>
#include <boost/capy/polystore_fwd.hpp>

#include <cstddef>
#include <cstdint>

namespace boost {
namespace http_proto {

// Forward declaration
class request_parser;
class response_parser;
class static_request;
class static_response;

//-----------------------------------------------

/** A parser for HTTP/1 messages.

    This parser uses a single block of memory allocated
    during construction and guarantees that it will
    never exceed the specified size. This space will be
    reused for parsing multiple HTTP messages (one
    message at a time).

    The allocated space will be utilized for the
    following purposes:

    @li Provide a mutable buffer sequence for reading
        raw input (for example, from a socket).
    @li Storing HTTP headers and provide a non-owning,
        read-only view that allows efficient access and
        iteration through header names and values.
    @li Offering O(1) access to important HTTP headers,
        including the request method, target, and
        response status code.
    @li Storing all or part of an HTTP message and
        provide the necessary interfaces for retrieving
        it.
    @li Taking ownership of user-provided elastic
        buffer and Sink objects.
    @li Storing the necessary state for inflate
        algorithms.

    The parser is strict. Any malformed inputs
    according to the documented HTTP ABNFs is treated
    as an unrecoverable error.

    @see
        @ref response_parser,
        @ref request_parser.
*/
class parser
{
public:
    /** The type of buffer returned from @ref prepare.
    */
    using mutable_buffers_type =
        boost::span<buffers::mutable_buffer const>;

    /** The type of buffer returned from @ref pull_body.
    */
    using const_buffers_type =
        boost::span<buffers::const_buffer const>;

    //--------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------

    /** Return true if a complete header has been
        parsed.

        @see
            @ref response_parser::get,
            @ref request_parser::get.
    */
    BOOST_HTTP_PROTO_DECL
    bool
    got_header() const noexcept;

    /** Return true if a complete message has been
        parsed.

        Calling @ref start prepares the parser to
        process the next message in the stream.

        @see
            @ref body,
            @ref start.
    */
    BOOST_HTTP_PROTO_DECL
    bool
    is_complete() const noexcept;

#if 0
    /** Return true if any input was committed.
    */
    BOOST_HTTP_PROTO_DECL
    bool
    got_some() const noexcept;

    /** Return true if the end of the stream was reached.

        The end of the stream is encountered
        when @ref commit_eof was called and there
        is no more data left to parse.

        When the end of stream is reached, the
        function @ref reset must be called
        to start parsing a new stream.
    */
    bool
    is_end_of_stream() const noexcept
    {
        return
            got_eof_ &&
            (st_ == state::reset || st_ >= state::complete_in_place);
    }
#endif

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    /** Prepare for a new stream.

        This function must be called before parsing  
        the first message in a new stream.
    */
    BOOST_HTTP_PROTO_DECL
    void
    reset() noexcept;

    /** Prepare for a new message.

        This function must be called before parsing
        a new message.

        @par Preconditions
        This function may only be called if it is the
        first message being read from the stream or if
        the previous message has been fully parsed.
    */
    BOOST_HTTP_PROTO_DECL
    void
    start();

    /** Prepares the input buffer.

        The returned buffer sequence will either
        reference the internal buffer or, if in use,
        the attached elastic buffer.

        A call to @ref commit is required to
        report the number of written bytes used,
        if any.

        @par Preconditions
        This function may only be called after a call
        to @ref parse completes with an error code
        equal to @ref condition::need_more_input.

        @par Exception Safety
        Strong guarantee.

        @return A non-empty mutable buffer.

        @see
            @ref commit,
            @ref commit_eof.
    */
    BOOST_HTTP_PROTO_DECL
    mutable_buffers_type
    prepare();

    /** Commit bytes to the input buffer

        After committing, a call to @ref parse
        is required to process the input.

        @par Preconditions
        @li `n <= buffers::size(this->prepare())`
        @li No previous call to @ref commit
        @li No previous call to @ref commit_eof

        @par Postconditions
        All buffer sequences previously obtained  
        from @ref prepare are invalidated.

        @par Exception Safety
        Strong guarantee.

        @param n The number of bytes written to
        the input buffer.

        @see
            @ref parse,
            @ref prepare.
    */
    BOOST_HTTP_PROTO_DECL
    void
    commit(
        std::size_t n);

    /** Indicate there will be no more input.

        @par Postconditions
        All buffer sequences previously obtained
        from @ref prepare are invalidated.

        @par Exception Safety
        Strong guarantee.

        @see
            @ref parse,
            @ref prepare.
    */
    BOOST_HTTP_PROTO_DECL
    void
    commit_eof();

    /** Parse pending input data

        This function attempts to parse the pending
        input data.

        This function returns immediately after the
        header is fully parsed. This is because certain
        operations, like @ref set_body_limit, must be
        performed before the body parsing starts. It is
        also more efficient to attach the body at this
        stage to avoid extra copy operations. The body
        parsing will begin in a subsequent call.

        If an error occurs during body parsing, the
        parsed header will remain valid and accessible.

        If @ref set_body was called previously, this
        function first tries to transfer available
        body data to the Sink or elastic buffer.

        When `ec == condition::need_more`, more input
        needs to be read into the internal buffer
        before continuing parsing.

        When `ec == error::end_of_stream`, all
        messages have been parsed, and the stream has
        closed cleanly. The parser can be reused for  
        a new stream after a call to @ref reset.

        @param ec Set to the error, if any occurred.

        @see
            @ref start,
            @ref prepare,
            @ref commit,
            @ref commit_eof.
    */
    BOOST_HTTP_PROTO_DECL
    void
    parse(
        system::error_code& ec);

    /** Attach an elastic buffer body.

        This function attaches the specified elastic
        buffer as the storage for the message body.

        A call to @ref parse is required after this
        function for the changes to take effect. This
        should automatically happen during the next
        IO layer call when reading the body.

        The parser takes ownership of the `eb` object and
        will destroy it when one of the following occurs:
        @li `this->is_complete() == true`
        @li An unrecoverable parsing error occurs
        @li The parser is destroyed

        @par Example
        @code
        response_parser pr{ctx};
        pr.start();

        read_header(stream, pr);

        std::string body;
        pr.set_body(buffers::string_buffer{&body});

        read(stream, pr);
        @endcode

        @par Preconditions
        @li `this->got_header() == true`
        @li No previous call to @ref set_body

        @par Constraints
        @code
        buffers::is_dynamic_buffer<ElasticBuffer>::value == true
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown if there is insufficient
        internal buffer to emplace the type-erased
        object of the ElasticBuffer.

        @throw std::length_error if there is
        insufficient internal buffer space to to
        emplace the type-erased object of the
        ElasticBuffer.

        @param eb The elastic buffer.

        @see
            @ref parse.
    */
    template<class ElasticBuffer>
    typename std::enable_if<
        ! detail::is_reference_wrapper<
            ElasticBuffer>::value &&
        ! is_sink<ElasticBuffer>::value>::type
    set_body(ElasticBuffer&& eb);

    /** Attach a reference to an elastic buffer body.

        This function attaches the specified elastic
        buffer reference as the storage for
        the message body.

        A call to @ref parse is required after this
        function for the changes to take effect. This
        should automatically happen during the next
        IO layer call when reading the body.

        Ownership is not transferred; the caller must
        ensure that the lifetime of the object
        reference by `eb` extends until:
        @li `this->is_complete() == true`
        @li An unrecoverable parsing error occurs
        @li The parser is destroyed

        @par Example
        @code
        response_parser pr{ctx};
        pr.start();

        read_header(stream, pr);

        std::string body;
        buffers::string_buffer buffer(&body);
        pr.set_body(std::ref(buffer));

        read(stream, pr);
        @endcode

        @par Preconditions
        @li `this->got_header() == true`
        @li No previous call to @ref set_body

        @par Constraints
        @code
        buffers::is_dynamic_buffer<ElasticBuffer>::value == true
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown if there is insufficient
        internal buffer to emplace the type-erased
        object of the ElasticBuffer.

        @throw std::length_error if there is
        insufficient internal buffer space to to
        emplace the type-erased object of the
        ElasticBuffer.

        @param eb A reference to an elastic buffer.

        @see
            @ref parse.
    */
    template<class ElasticBuffer>
    void set_body(
        std::reference_wrapper<ElasticBuffer> eb);

    /** Attach a Sink body.

        This function constructs a Sink for transferring
        the message body to it.

        A call to @ref parse is required after this
        function for the changes to take effect. This
        should automatically happen during the next
        IO layer call when reading the body.

        The parser destroys Sink object when:
        @li `this->is_complete() == true`
        @li An unrecoverable parsing error occurs
        @li The parser is destroyed

        @par Example
        @code
        response_parser pr{ctx};
        pr.start();

        read_header(stream, pr);

        pr.set_body<file_sink>("example.zip", file_mode::write_new);

        read(stream, pr);
        @endcode

        @par Preconditions
        @li `this->got_header() == true`
        @li No previous call to @ref set_body

        @par Constraints
        @code
        is_sink<Sink>::value == true
        @endcode

        @par Exception Safety
        Strong guarantee.
        Exceptions thrown if there is insufficient
        internal buffer to emplace the Sink object.

        @throw std::length_error if there is
        insufficient internal buffer space to to
        emplace the Sink object.

        @param args Arguments to be passed to the
        `Sink` constructor.

        @return A reference to the constructed Sink object.

        @see
            @ref sink,
            @ref file_sink,
            @ref parse.
    */
    template<
        class Sink,
        class... Args,
        class = typename std::enable_if<
            is_sink<Sink>::value>::type>
    Sink&
    set_body(Args&&... args);

    /** Sets a maximum body size for the current message.

        This value overrides the default limit
        defined by @ref config_base::body_limit,
        but applies *only* to the current message.
        The limit is automatically reset to the
        default for subsequent messages.

        @par Exception Safety
        Strong guarantee.

        @par Preconditions
        Can be called after @ref start and before
        parsing the message body. It can be called
        right after `this->got_header() == true`.

        @param n The body size limit in bytes.

        @see
            @ref config_base::body_limit.
    */
    BOOST_HTTP_PROTO_DECL
    void
    set_body_limit(std::uint64_t n);

    /** Return the available body data.

        The returned buffer may become invalid if
        any modifying member function is called.

        @par Example
        @code
        request_parser pr{ctx};
        pr.start();

        read_header(stream, pr);

        while(!pr.is_complete())
        {
            read_some(stream, pr);
            buffers::const_buffer_span cbs = pr.pull_body();
            // Do something with cbs ...
            pr.consume_body(buffer::buffer_size(cbs));
        }
        @endcode

        @par Preconditions
        @li `this->got_header() == true`
        @li No previous call to @ref set_body

        @par Exception Safety
        Strong guarantee.

        @return An instance of @ref const_buffers_type
        containing the parsed body data.

        @see
            @ref consume_body.
    */
    BOOST_HTTP_PROTO_DECL
    const_buffers_type
    pull_body();

    /** Consumes bytes from the available body data.

        @par Preconditions
        @code
        this->got_header() == true && n <= buffers::size(this->pull_body())
        @endcode

        @par Exception Safety
        Strong guarantee.

        @param n The number of bytes to consume from
        the available body data.

        @see
            @ref pull_body.
    */
    BOOST_HTTP_PROTO_DECL
    void
    consume_body(std::size_t n);

    /** Return the complete body as a contiguous buffer.

        This function is useful when the entire
        parsed message fits within the internal
        buffer allocated by the parser.

        @par Example
        @code
        request_parser pr{ctx};
        pr.start();

        read_header(stream, pr);
        // Read the entire body
        read(stream, pr);

        string_view body = pr.body();
        @endcode

        @par Exception Safety
        Strong guarantee.

        @par Preconditions
        @li `this->is_complete() == true`
        @li No previous call to @ref set_body
        @li No previous call to @ref consume_body

        @return A string view to the complete body
        data.

        @see
            @ref is_complete.
    */
    BOOST_HTTP_PROTO_DECL
    core::string_view
    body() const;

    /** Return any leftover data

        This is used to forward unconsumed data
        that could lie past the last message.
        For example on a CONNECT request there
        could be additional protocol-dependent
        data that we want to retrieve.

        @return A string view to leftover data.

        @see
            @ref metadata::upgrade, @ref metadata::connection.
    */
    // VFALCO rename to get_leftovers()?
    BOOST_HTTP_PROTO_DECL
    core::string_view
    release_buffered_data() noexcept;

private:
    friend class request_parser;
    friend class response_parser;
    class impl;

    BOOST_HTTP_PROTO_DECL ~parser();
    BOOST_HTTP_PROTO_DECL parser() noexcept;
    BOOST_HTTP_PROTO_DECL parser(parser&& other) noexcept;
    BOOST_HTTP_PROTO_DECL parser(detail::kind, prepared_parser_config);
    BOOST_HTTP_PROTO_DECL void assign(parser&& other) noexcept;
    BOOST_HTTP_PROTO_DECL void start_impl(bool);
    static_request const& safe_get_request() const;
    static_response const& safe_get_response() const;
    BOOST_HTTP_PROTO_DECL detail::workspace& ws() noexcept;
    BOOST_HTTP_PROTO_DECL bool is_body_set() const noexcept;
    BOOST_HTTP_PROTO_DECL void set_body_impl(
        buffers::any_dynamic_buffer&) noexcept;
    BOOST_HTTP_PROTO_DECL void set_body_impl(sink&) noexcept;

    static constexpr unsigned buffers_N = 8;
    impl* impl_;
};

//------------------------------------------------

/** Install the parser service.

    @par Example
    @code
    // default configuration settings for response_parser
    install_parser_service(ctx, response_parser::config{});

    response_parser pr(ctx);
    @endcode

    @par Exception Safety
    Strong guarantee.

    @throw std::invalid_argument If the service is
    already installed on the context.

    @param ctx Reference to the context on which
    the service should be installed.

    @param cfg Configuration settings for the
    @ref response_parser or @ref request_parser.

    @see
        @ref response_parser::config,
        @ref response_parser,
        @ref request_parser::config,
        @ref request_parser.
*/
BOOST_HTTP_PROTO_DECL
void
install_parser_service(
    capy::polystore& ctx,
    parser_config const& cfg);

} // http_proto
} // boost

#include <boost/http_proto/impl/parser.hpp>

#endif
