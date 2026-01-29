//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_JSON_JSON_SINK_HPP
#define BOOST_HTTP_JSON_JSON_SINK_HPP

#include <boost/http/config.hpp>

#include <boost/capy/buffers.hpp>
#include <boost/capy/concept/const_buffer_sequence.hpp>
#include <boost/capy/ex/immediate.hpp>
#include <boost/capy/io_result.hpp>
#include <boost/json/stream_parser.hpp>
#include <boost/json/value.hpp>

namespace boost {
namespace http {

/** A sink for streaming JSON data to a parser.

    This class wraps a `boost::json::stream_parser` and satisfies the
    @ref capy::WriteSink concept, enabling incremental JSON parsing
    from any data source that produces buffer sequences.

    Since JSON parsing is synchronous, all operations return
    @ref capy::immediate awaitables with zero suspension overhead.

    @par Example
    @code
    json_sink sink;

    // Write JSON data incrementally
    auto [ec1, n1] = co_await sink.write(capy::make_buffer("{\"key\":"));
    auto [ec2, n2] = co_await sink.write(capy::make_buffer("42}"), true);

    // Or use write_eof() separately
    auto [ec3] = co_await sink.write_eof();

    // Retrieve the parsed value
    json::value v = sink.release();
    @endcode

    @par Thread Safety
    Distinct objects: Safe.
    Shared objects: Unsafe.

    @see capy::WriteSink, json::stream_parser
*/
class json_sink
{
    json::stream_parser parser_;

public:
    /** Default constructor.

        Constructs a sink with a default-initialized stream parser.
    */
    json_sink() = default;

    /** Constructor with parse options.

        @param opt Options controlling JSON parsing behavior.
    */
    explicit
    json_sink(json::parse_options const& opt)
        : parser_(json::storage_ptr(), opt)
    {
    }

    /** Constructor with storage and parse options.

        @param sp The storage to use for parsed values.
        @param opt Options controlling JSON parsing behavior.
    */
    json_sink(
        json::storage_ptr sp,
        json::parse_options const& opt = {})
        : parser_(std::move(sp), opt)
    {
    }

    /** Write data to the JSON parser.

        Writes all bytes from the buffer sequence to the stream parser.

        @param buffers Buffer sequence containing JSON data.

        @return An awaitable yielding `(error_code,std::size_t)`.
            On success, returns the total bytes written.
    */
    template<capy::ConstBufferSequence CB>
    capy::immediate<capy::io_result<std::size_t>>
    write(CB const& buffers)
    {
        return write(buffers, false);
    }

    /** Write data with optional end-of-stream.

        Writes all bytes from the buffer sequence to the stream parser.
        If @p eof is true, also finishes parsing.

        @param buffers Buffer sequence containing JSON data.
        @param eof If true, signals end of JSON data after writing.

        @return An awaitable yielding `(error_code,std::size_t)`.
            On success, returns the total bytes written.
    */
    template<capy::ConstBufferSequence CB>
    capy::immediate<capy::io_result<std::size_t>>
    write(CB const& buffers, bool eof)
    {
        system::error_code ec;
        std::size_t total = 0;
        auto const end = capy::end(buffers);
        for(auto it = capy::begin(buffers); it != end; ++it)
        {
            capy::const_buffer buf(*it);
            auto n = parser_.write(
                static_cast<char const*>(buf.data()),
                buf.size(),
                ec);
            total += n;
            if(ec.failed())
                return {ec, total};
        }

        if(eof)
        {
            parser_.finish(ec);
            if(ec.failed())
                return {ec, total};
        }

        return capy::ready(total);
    }

    /** Signal end of JSON data.

        Finishes parsing and validates the JSON is complete.

        @return An awaitable yielding `(error_code)`.
    */
    capy::immediate<capy::io_result<>>
    write_eof()
    {
        system::error_code ec;
        parser_.finish(ec);
        if(ec.failed())
            return {ec};
        return {};
    }

    /** Check if parsing is complete.

        @return `true` if a complete JSON value has been parsed.
    */
    bool
    done() const noexcept
    {
        return parser_.done();
    }

    /** Release the parsed JSON value.

        Returns the parsed value and resets the parser for reuse.

        @par Preconditions
        `this->done() == true`

        @return The parsed JSON value.
    */
    json::value
    release()
    {
        return parser_.release();
    }

    /** Reset the parser for a new JSON value.

        Clears all state and prepares to parse a new value.
    */
    void
    reset()
    {
        parser_.reset();
    }
};

} // namespace http
} // namespace boost

#endif
