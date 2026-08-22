# Parser redesign

## Summary

This document describes a redesign of the library’s message parser. As in the existing design, the parser uses a single block of memory allocated at construction time and never exceeds it. That memory is reused across messages, and headers are parsed in place. The parser remains a strictly sans-I/O state machine, while stream handling moves to a separate `message_reader`. Message bodies can be retrieved without copying, including by reading directly from the stream into caller-supplied memory. Chunked trailers are now accessible, and per-message buffer compaction is optimized so that pipelined messages can be read and parsed in place without an extra `memmove`.

The design is not speculative: it is implemented and tested in the Burl project (<https://github.com/cppalliance/burl>).

## The parser interface

```cpp
class parser
{
public:
    /// Settings which apply for the life of
    /// the parser.
    struct config
    {
        /// The limits enforced while parsing
        /// a header.
        header_limits hdr_limits;

        /// The space reserved for buffering
        /// received octets.
        std::size_t in_buffer = 64 * 1024;

        /// The space reserved for decoded
        /// output.
        std::size_t dec_buffer = 8 * 1024;

        /// The maximum body size.
        std::uint64_t body_limit = std::uint64_t(-1);

        /// Whether to decode the body according
        /// to its Content-Encoding.
        bool decode = true;
    };

    /// Return true if the header has been parsed.
    bool
    got_header() const noexcept;

    /// Return true if the entire message
    /// has arrived.
    bool
    got_body() const noexcept;

    /// Return true if octets are buffered
    /// past the message.
    bool
    has_buffered_data() const noexcept;

    /// Return the unconsumed octets in the
    /// buffer (e.g. Upgrade: websocket).
    std::array<capy::const_buffer, 2>
    buffered_data() const noexcept;

    /// Prepare for a new stream, discarding
    /// all state and buffered octets.
    void
    reset() noexcept;

    /// Set the maximum body size, overriding
    /// config::body_limit.
    void
    set_body_limit(std::uint64_t n) noexcept;

    /// Return the buffer region for
    /// receiving octets.
    std::array<capy::mutable_buffer, 2>
    prepare() noexcept;

    /// Report octets received into the
    /// region returned by prepare.
    void
    commit(std::size_t n) noexcept;

    /// Report the end of the stream.
    void
    commit_eof() noexcept;

    /// Return how many body octets may be
    /// read straight into caller memory.
    std::size_t
    direct_capacity() const noexcept;

    /// Report octets received directly
    /// into caller memory.
    void
    commit_direct(std::size_t n) noexcept;

    /// Parse the header, returning as soon
    /// as it is complete.
    system::result<void, std::error_code>
    parse_header();

    /// Flatten the body in place, de-chunking
    /// chunked bodies, and return a view of it.
    system::result<std::string_view, std::error_code>
    flatten_body();

    /// Copy body octets, or the decoder's
    /// output, into caller memory.
    template<capy::MutableBufferSequence MB>
    system::result<std::size_t, std::error_code>
    read_some(MB const& buffers);

    /// Assign `dest` with descriptors into the
    /// parser's own buffers.
    system::result<
        std::span<capy::const_buffer>,
        std::error_code>
    pull(std::span<capy::const_buffer> dest);

    /// Release body octets returned by pull.
    void
    consume(std::size_t n) noexcept;

    /// Append the trailer fields of a
    /// chunked payload to `f`.
    system::result<void, std::error_code>
    parse_trailer(fields_base& f);
};

class request_parser
    : public parser
{
public:
    /// Constructor.
    request_parser() = default;

    /// Constructor.
    explicit
    request_parser(config const& cfg);

    /// Prepare for a new message; octets
    /// already received for it are retained.
    void
    start();

    /// Return the parsed header.
    request_head_base const&
    get() const;
};

class response_parser
    : public parser
{
public:
    /// Constructor.
    response_parser() = default;

    /// Constructor.
    explicit
    response_parser(config const& cfg);

    /// Prepare for a new message; `head`
    /// states it answers a HEAD request.
    void
    start(bool head = false);

    /// Return the parsed header.
    response_head_base const&
    get() const;
};
```

## Direct reads from the stream into a user-provided buffer

The new `direct_capacity` and `commit_direct` interfaces allow users to query the parser state and avoid unnecessary copies when the body type allows it and all internally buffered data has already been consumed. This ensures that direct reads do not interleave with data already buffered by the parser.

This feature is particularly useful for operations such as file downloads, where the body is not encoded and its size is known in advance, so chunked encoding is not required. In these cases, the body can be read directly into the user's buffer, eliminating an otherwise unnecessary copy.

The following is a possible implementation of a `read_some` algorithm that takes advantage of direct reads:

```cpp
template<
    capy::ReadStream S,
    capy::MutableBufferSequence MB>
capy::io_task<std::size_t>
read_some_(
    S& stream,
    parser& pr,
    MB buffers)
{
    for(;;)
    {
        // Try to read the internally buffered data first
        auto const r = pr.read_some(buffers);
        if(r.has_value())
            co_return { std::error_code(), *r };
        if(r.error() != http::condition::need_more_input)
            co_return { r.error(), 0 };

        // Check if the parser state permits a direct read
        if(auto const limit = pr.direct_capacity(); limit != 0)
        {
            // Read directly from the stream, with the buffer
            // limited to the size determined by the parser
            auto [ec, n] = co_await stream.read_some(
                capy::buffer_slice(buffers, 0, limit));
            pr.commit_direct(n);
            if(ec == capy::cond::eof)
                pr.commit_eof();
            else if(ec)
                co_return { ec, n };
            if(n != 0)
                co_return { std::error_code(), n };
            continue;
        }

        // Refill the parser and try again
        if(auto [ec] = co_await refill(stream, pr); ec)
            co_return { ec, 0 };
    }
}
```

## Trailer fields are no longer discarded

Unlike the existing parser, the new parser does not discard trailer fields. Instead, it allows users to append them to a separate, user-provided field container. Although these fields could theoretically be read in place, like the header fields, we chose not to do so because it would add complexity and require additional parser state for a rarely used feature.

## No need for a parser configuration service

The parser reads its configuration only during construction. At that point, the configured buffer sizes are determined and the single allocation is performed. The limit parameters are then stored locally: `header_limits` is passed directly to the `head_parser`, which keeps its own copy, while `body_limit` is stored as a member of the parser itself. Since the configuration is never consulted again after construction, there is no need to retain a shared reference or introduce a separate configuration service.

## Quality of the implementation

### Per-message buffer compaction is optimized

In the new implementation, the parser starts parsing the next header at the address where its octets already lie inside the circular buffer. Octets move only when actually required:

- if the header cannot complete below the buffer's ceiling, the buffered octets slide to the front once and parsing resumes via `head_parser::rebase`;
- after the header completes with the body still in flight, the region slides to the front once to maximize contiguous receive space.

As a result, a sequence of pipelined messages that fit in the buffer (the common fast path) parses with no relocation at all.

### Chunked messages are read without compaction

The new implementation walks the chunked messages and assigns their bodies to the user-provided buffer span. As a result, chunked message bodies can be read in place without any copying or memory movement.

## The `message_reader` interface

```cpp
template<capy::ReadStream S>
class message_reader
{
public:
    /// Constructor; the stream and parser
    /// must outlive the reader.
    message_reader(S* stream, parser* pr) noexcept;

    /// Parse the header, reading from the
    /// stream until it is complete.
    capy::io_task<>
    read_header();

    /// Read the complete body into the
    /// parser's buffer and return a view of it.
    capy::io_task<std::string_view>
    read_body();

    /// Read body octets into `buffers`.
    template<capy::MutableBufferSequence MB>
    capy::io_task<std::size_t>
    read_some(MB buffers);

    /// Read until `buffers` is full or the
    /// body is complete.
    template<capy::MutableBufferSequence MB>
    capy::io_task<std::size_t>
    read(MB buffers);

    /// Assign `dest` with descriptors into the
    /// parser's own buffers.
    capy::io_task<std::span<capy::const_buffer>>
    pull(std::span<capy::const_buffer> dest);

    /// Release body octets returned by pull.
    void
    consume(std::size_t n) noexcept;
};
```

The rationale for the existence of `message_reader` is to satisfy the `capy::ReadStream`, `capy::ReadSource` and `capy::BufferSource` concepts, so it composes with generic stream algorithms.

## Open questions and possibilities regarding `message_reader`

While the current design has proven itself in the Burl project, there are still some open questions worth discussing in this section.

### Should we use `capy::any_read_stream` instead of templating on the stream type?

Using a `capy::any_read_stream` instance would make `message_reader` a concrete type. If we consider `message_reader` to be a type that is exposed to users, for example as part of the Beast2 request handler interface, then using a concrete type may make more sense.

There is also the possibility of providing both a templated implementation and a concrete alias:

```cpp
template<typename Stream>
class basic_message_reader;

using message_reader = basic_message_reader<capy::any_read_stream>;
```

### Should we add `request_reader` and `response_reader`?

If we provide request- and response-specific versions, the same instance could provide access to the corresponding parser as well as the header section:

```cpp
class request_reader
    : public message_reader
{
public:
    /// Return the parser.
    request_parser&
    get_parser();

    /// Parse and return the header.
    capy::io_task<request_head_base const&>
    read_header();
};
```

## Related links

Reference documentation for the Burl implementation:

- [`parser`](https://develop.burl.cpp.al/burl/reference/boost/burl/parser.html)
- [`request_parser`](https://develop.burl.cpp.al/burl/reference/boost/burl/request_parser.html)
- [`response_parser`](https://develop.burl.cpp.al/burl/reference/boost/burl/response_parser.html)
- [`message_reader`](https://develop.burl.cpp.al/burl/reference/boost/burl/message_reader.html)
