//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_PARSER_HPP
#define BOOST_HTTP_PARSER_HPP

#include <boost/http/config.hpp>
#include <boost/http/detail/header.hpp>
#include <boost/http/detail/type_traits.hpp>
#include <boost/http/error.hpp>

#include <boost/capy/buffers/buffer_copy.hpp>
#include <boost/capy/buffers/buffer_pair.hpp>
#include <boost/capy/buffers/slice.hpp>
#include <boost/capy/concept/read_stream.hpp>
#include <boost/capy/concept/write_sink.hpp>
#include <boost/capy/cond.hpp>
#include <boost/capy/error.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/capy/task.hpp>
#include <boost/core/span.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace boost {
namespace http {

// Forward declaration
class request_parser;
class response_parser;
class static_request;
class static_response;

//------------------------------------------------

/** A parser for HTTP/1 messages.

    This parser uses a single block of memory allocated
    during construction and guarantees it will never
    exceed the specified size. This space is reused for
    parsing multiple HTTP messages ( one at a time ).

    The allocated space is used for:

    @li Buffering raw input from a socket
    @li Storing HTTP headers with O(1) access to
        method, target, and status code
    @li Storing all or part of an HTTP message body
    @li Storing state for inflate algorithms

    The parser is strict. Any malformed input according
    to the HTTP ABNFs is treated as an unrecoverable
    error.

    @see
        @ref response_parser,
        @ref request_parser.
*/
class parser
{
public:
    template<capy::ReadStream Stream>
    class read_source_adapter;

    /// Buffer type returned from @ref prepare.
    using mutable_buffers_type =
        boost::span<capy::mutable_buffer const>;

    /// Buffer type returned from @ref pull_body.
    using const_buffers_type =
        boost::span<capy::const_buffer const>;

    //--------------------------------------------
    //
    // Observers
    //
    //--------------------------------------------

    /// Check if a complete header has been parsed.
    BOOST_HTTP_DECL
    bool
    got_header() const noexcept;

    /// Check if a complete message has been parsed.
    BOOST_HTTP_DECL
    bool
    is_complete() const noexcept;

    //--------------------------------------------
    //
    // Modifiers
    //
    //--------------------------------------------

    /// Prepare for a new stream.
    BOOST_HTTP_DECL
    void
    reset() noexcept;

    /** Prepare for a new message.

        @par Preconditions
        Either this is the first message in the stream,
        or the previous message has been fully parsed.
    */
    BOOST_HTTP_DECL
    void
    start();

    /** Return a buffer for reading input.

        After writing to the buffer, call @ref commit
        with the number of bytes written.

        @par Preconditions
        @ref parse returned @ref condition::need_more_input.

        @par Postconditions
        A call to @ref commit or @ref commit_eof is
        required before calling @ref prepare again.

        @par Exception Safety
        Strong guarantee.

        @return A non-empty mutable buffer.

        @see @ref commit, @ref commit_eof.
    */
    BOOST_HTTP_DECL
    mutable_buffers_type
    prepare();

    /** Commit bytes to the input buffer.

        @par Preconditions
        @li `n <= capy::buffer_size( this->prepare() )`
        @li No prior call to @ref commit or @ref commit_eof
            since the last @ref prepare

        @par Postconditions
        Buffers from @ref prepare are invalidated.

        @par Exception Safety
        Strong guarantee.

        @param n The number of bytes written.

        @see @ref parse, @ref prepare.
    */
    BOOST_HTTP_DECL
    void
    commit(
        std::size_t n);

    /** Indicate end of input.

        Call this when the underlying stream has closed
        and no more data will arrive.

        @par Postconditions
        Buffers from @ref prepare are invalidated.

        @par Exception Safety
        Strong guarantee.

        @see @ref parse, @ref prepare.
    */
    BOOST_HTTP_DECL
    void
    commit_eof();

    /** Parse pending input data.

        Returns immediately after the header is fully
        parsed to allow @ref set_body_limit to be called
        before body parsing begins. If an error occurs
        during body parsing, the parsed header remains
        valid and accessible.

        When `ec == condition::need_more_input`, read
        more data and call @ref commit before calling
        this function again.

        When `ec == error::end_of_stream`, the stream
        closed cleanly. Call @ref reset to reuse the
        parser for a new stream.

        @param ec Set to the error, if any occurred.

        @see @ref start, @ref prepare, @ref commit.
    */
    BOOST_HTTP_DECL
    void
    parse(
        system::error_code& ec);

    /** Set maximum body size for the current message.

        Overrides @ref parser_config::body_limit for this
        message only. The limit resets to the default
        for subsequent messages.

        @par Preconditions
        `this->got_header() == true` and body parsing
        has not started.

        @par Exception Safety
        Strong guarantee.

        @param n The body size limit in bytes.

        @see @ref parser_config::body_limit.
    */
    BOOST_HTTP_DECL
    void
    set_body_limit(std::uint64_t n);

    /** Return available body data.

        Use this to incrementally process body data.
        Call @ref consume_body after processing to
        release the buffer space.

        @par Example
        @code
        request_parser pr( ctx );
        pr.start();
        co_await pr.read_header( stream );

        while( ! pr.is_complete() )
        {
            co_await read_some( stream, pr );
            auto cbs = pr.pull_body();
            // process cbs ...
            pr.consume_body( capy::buffer_size( cbs ) );
        }
        @endcode

        @par Preconditions
        `this->got_header() == true`

        @par Postconditions
        The returned buffer is invalidated by any
        modifying member function.

        @par Exception Safety
        Strong guarantee.

        @return Buffers containing available body data.

        @see @ref consume_body.
    */
    BOOST_HTTP_DECL
    const_buffers_type
    pull_body();

    /** Consume bytes from available body data.

        @par Preconditions
        `n <= capy::buffer_size( this->pull_body() )`

        @par Exception Safety
        Strong guarantee.

        @param n The number of bytes to consume.

        @see @ref pull_body.
    */
    BOOST_HTTP_DECL
    void
    consume_body(std::size_t n);

    /** Return the complete body.

        Use this when the entire message fits within
        the parser's internal buffer.

        @par Example
        @code
        request_parser pr( ctx );
        pr.start();
        co_await pr.read_header( stream );
        // ... read entire body ...
        core::string_view body = pr.body();
        @endcode

        @par Preconditions
        @li `this->is_complete() == true`
        @li No previous call to @ref consume_body

        @par Exception Safety
        Strong guarantee.

        @return A string view of the complete body.

        @see @ref is_complete.
    */
    BOOST_HTTP_DECL
    core::string_view
    body() const;

    /** Return unconsumed data past the last message.

        Use this after an upgrade or CONNECT request
        to retrieve protocol-dependent data that
        follows the HTTP message.

        @return A string view of leftover data.

        @see @ref metadata::upgrade, @ref metadata::connection.
    */
    BOOST_HTTP_DECL
    core::string_view
    release_buffered_data() noexcept;

    /** Asynchronously read the HTTP headers.

        Reads from the stream until the headers are
        complete or an error occurs.

        @par Preconditions
        @li @ref reset has been called
        @li @ref start has been called

        @param stream The stream to read from.

        @return An awaitable yielding `(error_code)`.

        @see @ref read.
    */
    template<capy::ReadStream Stream>
    capy::task<capy::io_result<>>
    read_header(Stream& stream);

    /** Asynchronously read body data into buffers.

        Reads from the stream and copies body data into
        the provided buffers with complete-fill semantics.
        Returns `capy::error::eof` when the body is complete.

        @par Preconditions
        @li @ref reset has been called
        @li @ref start has been called

        @param stream The stream to read from.

        @param buffers The buffers to read into.

        @return An awaitable yielding `(error_code,std::size_t)`.

        @see @ref read_header.
    */
    template<capy::ReadStream Stream, capy::MutableBufferSequence MB>
    capy::task<capy::io_result<std::size_t>>
    read(Stream& stream, MB const& buffers);

    /** Return an adapter for reading body data.

        The returned adapter satisfies @ref capy::ReadSource.
        Buffers are filled completely before returning, or
        `capy::error::eof` is returned when the body is
        complete.

        @par Example
        @code
        response_parser pr( ctx );
        pr.reset();
        pr.start();
        co_await pr.read_header( socket );

        auto body = pr.as_read_source( socket );
        char buf[4096];
        auto [ ec, n ] = co_await body.read( capy::mutable_buffer( buf ) );
        @endcode

        @param stream The stream to read from.

        @return An adapter satisfying @ref capy::ReadSource.

        @see @ref read_header, @ref capy::ReadSource.
    */
    template<capy::ReadStream Stream>
    read_source_adapter<Stream>
    as_read_source(Stream& stream);

    /** Read body from stream and push to a WriteSink.

        Reads body data from the stream and pushes each chunk to
        the sink. The sink must consume all bytes from each write.

        @param stream The stream to read body data from.

        @param sink The sink to receive body data.

        @return An awaitable yielding `(error_code)`.

        @see WriteSink.
    */
    template<capy::WriteSink Sink>
    capy::task<capy::io_result<>>
    read(capy::ReadStream auto& stream, Sink&& sink);

private:
    friend class request_parser;
    friend class response_parser;
    class impl;

    BOOST_HTTP_DECL ~parser();
    BOOST_HTTP_DECL parser() noexcept;
    BOOST_HTTP_DECL parser(parser&& other) noexcept;
    BOOST_HTTP_DECL parser(
        std::shared_ptr<parser_config_impl const> cfg,
        detail::kind k);
    BOOST_HTTP_DECL void assign(parser&& other) noexcept;

    BOOST_HTTP_DECL
    void
    start_impl(bool);

    static_request const&
    safe_get_request() const;

    static_response const&
    safe_get_response() const;

    impl* impl_;
};

/** Adapter for reading body data from a parser.

    Satisfies @ref capy::ReadSource with complete-fill
    semantics.

    @see @ref parser::as_read_source.
*/
template<capy::ReadStream Stream>
class parser::read_source_adapter
{
    Stream& stream_;
    parser& pr_;

public:
    /// Construct an adapter for reading body data.
    read_source_adapter(Stream& stream, parser& pr) noexcept
        : stream_(stream)
        , pr_(pr)
    {
    }

    /** Asynchronously read body data into buffers.

        @return An awaitable yielding `(error_code,std::size_t)`.
    */
    template<capy::MutableBufferSequence MB>
    capy::task<capy::io_result<std::size_t>>
    read(MB const& buffers);
};

template<capy::ReadStream Stream>
capy::task<capy::io_result<>>
parser::
read_header(Stream& stream)
{
    system::error_code ec;
    for(;;)
    {
        parse(ec);

        if(got_header())
            co_return {};

        if(ec != condition::need_more_input)
            co_return {ec};

        auto mbs = prepare();
        auto [read_ec, n] = co_await stream.read_some(mbs);

        if(read_ec == capy::cond::eof)
            commit_eof();
        else if(!read_ec.failed())
            commit(n);
        else
            co_return {read_ec};
    }
}

template<capy::ReadStream Stream, capy::MutableBufferSequence MB>
capy::task<capy::io_result<std::size_t>>
parser::
read(Stream& stream, MB const& buffers)
{
    if(capy::buffer_empty(buffers))
        co_return {{}, 0};

    std::size_t total = 0;
    auto dest = buffers;

    for(;;)
    {
        system::error_code ec;
        parse(ec);

        if(got_header())
        {
            auto body_data = pull_body();
            if(capy::buffer_size(body_data) > 0)
            {
                std::size_t copied = capy::buffer_copy(dest, body_data);
                consume_body(copied);
                total += copied;
                dest = capy::sans_prefix(dest, copied);

                if(capy::buffer_empty(dest))
                    co_return {{}, total};
            }

            if(is_complete())
                co_return {capy::error::eof, total};
        }

        if(ec == condition::need_more_input)
        {
            auto mbs = prepare();
            auto [read_ec, n] = co_await stream.read_some(mbs);

            if(read_ec == capy::cond::eof)
                commit_eof();
            else if(!read_ec.failed())
                commit(n);
            else
                co_return {read_ec, total};

            continue;
        }

        if(ec.failed())
            co_return {ec, total};
    }
}

template<capy::ReadStream Stream>
template<capy::MutableBufferSequence MB>
capy::task<capy::io_result<std::size_t>>
parser::read_source_adapter<Stream>::
read(MB const& buffers)
{
    return pr_.read(stream_, buffers);
}

template<capy::ReadStream Stream>
parser::read_source_adapter<Stream>
parser::
as_read_source(Stream& stream)
{
    return read_source_adapter<Stream>(stream, *this);
}

template<capy::WriteSink Sink>
capy::task<capy::io_result<>>
parser::
read(capy::ReadStream auto& stream, Sink&& sink)
{
    for(;;)
    {
        system::error_code ec;
        parse(ec);

        if(got_header())
        {
            auto body_data = pull_body();
            if(capy::buffer_size(body_data) > 0)
            {
                auto [write_ec, n] = co_await sink.write(body_data);
                if(write_ec.failed())
                    co_return {write_ec};
                consume_body(n);
            }

            if(is_complete())
            {
                auto [eof_ec] = co_await sink.write_eof();
                co_return {eof_ec};
            }
        }

        if(ec == condition::need_more_input)
        {
            auto mbs = prepare();
            auto [read_ec, n] = co_await stream.read_some(mbs);

            if(read_ec == capy::cond::eof)
                commit_eof();
            else if(!read_ec.failed())
                commit(n);
            else
                co_return {read_ec};

            continue;
        }

        if(ec.failed())
            co_return {ec};
    }
}

} // http
} // boost

#endif
