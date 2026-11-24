# Serializer redesign

## Summary

This document describes a redesign of the library's message serializer. As in the existing design, the serializer uses a single block of memory allocated at construction time and never exceeds it. The serializer remains a strictly sans-I/O component, while stream handling moves to a separate `message_writer`. Caller-owned memory without copying; small pieces are coalesced in a staging buffer that the caller can also write into directly. Every framing decision is deferred to the first call that produces output or hands out staging memory, so the caller may keep shaping the start line and the framing fields until then. Chunked or close-delimited body whose total size becomes known before transfer begins is sent with an explicit `Content-Length` instead, and an HTTP/1.1 message with no framing fields is sent chunked rather than close-delimited, keeping the connection reusable. Chunked trailers can now be sent, and the accounting remains exact when an operation is cancelled mid-write, so an interrupted message can be resumed.

The design is not speculative: it is implemented and tested in the Burl project (<https://github.com/cppalliance/burl>).

## The serializer interface

```cpp
/// Per-coding settings for the content
/// encoders.
struct encoder_config
{
    /// level, window_bits, mem_level
    struct zlib_settings;

    /// quality, lgwin, lgblock, mode
    struct brotli_settings;

    /// level, window_log, strategy
    struct zstd_settings;

    zlib_settings zlib = {};
    brotli_settings brotli = {};
    zstd_settings zstd = {};
};

class serializer
{
public:
    /// Settings that apply for the lifetime of
    /// the serializer.
    struct config
    {
        /// The space reserved for staging
        /// body octets.
        std::size_t stage_buffer = 64 * 1024;

        /// The free staging capacity below
        /// which should_drain reports true.
        std::size_t min_prepare = 4 * 1024;

        /// Supplied body data at least this
        /// large is framed by reference,
        /// without copying.
        std::size_t min_direct = 2 * 1024;

        /// The space reserved for the encoder
        /// input stage; unused when no encoder
        /// is installed.
        std::size_t enc_buffer = 8 * 1024;

        /// Bodies smaller than this may skip
        /// encoding entirely.
        std::size_t enc_threshold = 4 * 1024;

        /// The content encoder settings; when
        /// null, no body is encoded.
        std::shared_ptr<encoder_config const> encoder;
    };

    /// Constructor; performs the single
    /// allocation.
    explicit
    serializer(config const& cfg);

    /// Return true if every octet of the
    /// message, including any trailer, has
    /// been consumed.
    bool
    is_done() const noexcept;

    /// Return true if every octet of the
    /// serialized header has been consumed.
    bool
    is_header_done() const noexcept;

    /// Return true if staged octets should
    /// be drained.
    bool
    should_drain() const noexcept;

    /// Start a message. The framing is selected
    /// from msg->payload() on the first call
    /// to prepare or frame, and the caller
    /// may modify msg until then. `head`
    /// serializes the response to a HEAD
    /// request, emitting only the header.
    void
    start(
        message_head_base* msg,
        bool head = false);

    /// Set the trailer fields, serialized
    /// after the final chunk.
    void
    set_trailer(fields_base const* t) noexcept;

    /// Return a buffer for writing body
    /// octets into the staging area.
    std::span<capy::mutable_buffer>
    prepare(std::span<capy::mutable_buffer> dest);

    /// Make octets written into the region
    /// returned by prepare part of the body.
    void
    commit(std::size_t n) noexcept;

    /// Assign `dest` with descriptors for the
    /// octets to transfer next, framing the
    /// supplied body octets.
    template<capy::ConstBufferSequence CB>
    system::result<
        std::span<capy::const_buffer const>,
        std::error_code>
    frame(
        std::span<capy::const_buffer> dest,
        CB const& buffers,
        bool more);

    /// Equivalent to frame with an empty
    /// buffer sequence.
    system::result<
        std::span<capy::const_buffer const>,
        std::error_code>
    frame(
        std::span<capy::const_buffer> dest,
        bool more);

    /// Report transferred octets; return how
    /// many supplied body octets were accepted.
    std::size_t
    consume(std::size_t n) noexcept;
};
```

## Zero-copy framing of user-provided buffers

The new serializer interface allows the user to provide their buffers during the framing step. Because serialization and framing happen in the same call, the serializer can use the supplied buffers directly as part of the destination buffer sequence that is expected to be written to the stream. This simple change provides a streaming interface for writing body data from an external source without copying it into the serializer's internal buffer.

The following is a possible implementation of a `write_some` algorithm using this interface:

```cpp
template<
    capy::WriteStream S,
    capy::ConstBufferSequence CB>
capy::io_task<std::size_t>
write_some(
    S& stream,
    serializer& sr,
    CB buffers,
    bool more)
{
    capy::consuming_buffers cb(buffers);
    capy::const_buffer dest[16];
    std::size_t total = 0;
    for(;;)
    {
        auto const bufs = sr.frame(dest, cb.data(), more);
        if(bufs.has_error())
            co_return { bufs.error(), total };
        auto [ec, n] = co_await stream.write_some(*bufs);
        auto const k = sr.consume(n);
        cb.consume(k);
        total += k;
        if(ec || n == 0)
            co_return { ec, total };
    }
}
```

## Optimization of I/O-layer write operations

There are two new configuration parameters that allow optimization of the number of write operations at the I/O layer:

- `config::min_direct` sets a threshold for buffer sizes that the serializer frames directly in the destination buffers for writing. Buffers smaller than this threshold are copied into the serializer's internal buffer, where they wait for more data or for the end of the message.

- `config::min_prepare` determines the minimum amount of internal buffer space that the serializer provides to the user in calls to `prepare`. As long as the serializer can satisfy that requirement, it does not hint to the I/O layer that it should flush, thereby reducing the number of write operations.

## The message head is read on first use

`start` only attaches the message, nothing is derived from it at that point. Every head-derived decision is deferred until the first call to `prepare` or `frame`. Until then, the message head belongs to the caller, who may continue adjusting the status line and headers. The serializer acts on the final values.

A server worker owns one serializer per connection, exposes the response body to handlers through the `message_writer` or a type-erased sink wired at construction, and must therefore start the serializer before dispatching. The response head may contain nothing more than a status line and a `Connection` field, and handlers can then shape the response up to their first write. Under the previous contract, framing state pinned at `start` could disagree with the header octets emitted later from the live head, producing messages whose wire framing contradicted their own headers. Deferring the read ensures that the two always describe the same head.

After the first call to `prepare` or `frame`, the head is frozen from the caller's perspective until the message completes or is abandoned. The serializer itself retains the right to revise the framing fields until the first header octet is consumed, which enables the late framing described next.

## Late framing decisions

Because the serializer does not request a flush until there is a reason to do so (e.g. enough body data has accumulated, the end of the body has been declared, etc.), it may still alter the framing-related fields of the header:

- When the complete body arrives before the first write, chunked or close-delimited framing is replaced by an explicit `Content-Length`, and `chunked` is removed from the `Transfer-Encoding` field.

- An HTTP/1.1 message with no framing fields, which would otherwise be close-delimited at the cost of the connection, is given the chunked transfer coding before the header is transferred. HTTP/1.0 messages, and messages that name a transfer coding of their own, keep close-delimited framing.

- When a content encoder is selected but the complete body is smaller than `config::enc_threshold`, the serializer may skip encoding entirely, remove the `Content-Encoding` field, and serialize the body unencoded.

All rewrites require that no header octet has been consumed. Setting a trailer suppresses the `Content-Length` rewrite because trailers require chunked coding, a trailer set on a message with no framing fields therefore rides the chunked upgrade. Once transfer of the header has begun, the message head is never modified.

In practice, this means the caller never has to choose a framing for a response: handing the whole body to a single `write_eof` produces a `Content-Length`-framed message even when the caller never computed the size, while true streaming goes out chunked automatically, and the connection stays reusable either way:

```CPP
response_head res; // no framing fields set

serializer sr(cfg);
message_writer writer(&stream, &sr);
sr.start(&res);

std::string body = make_response();

// The complete body arrives before the
// first write, so the message goes out
// with an explicit Content-Length.
auto [ec, n] = co_await writer.write_eof(
    capy::make_buffer(body));
```

## Trailer fields can now be sent

The existing serializer has no way to emit trailer fields. In the new design, `set_trailer` installs a caller-owned field container whose wire image is serialized after the final chunk of a chunked body; with any other framing, the trailer is ignored.

## Content encoders are selected through services

When `config::encoder` is set, the serializer selects the content coding from the message passed to `start`: a body whose `Content-Encoding` names `gzip`, `deflate`, `br`, or `zstd` is encoded as it is serialized, using the encode service installed for that coding in the system context and the per-coding settings from `encoder_config`, and the encoder's output is framed in place of the body. A coding without an installed service, or one the serializer does not know, leaves the body as supplied. As described in the late-framing section above, the serializer may skip encoding and remove the `Content-Encoding` field entirely; a user can set `config::enc_threshold` to zero to force encoding for all body sizes.

## No need for a serializer configuration service

The serializer reads its configuration only during construction, at which point the single buffer allocation is performed and the tuning values are stored as members. Since the configuration is never consulted again, there is no need for a shared reference or a configuration service.

## No need for storing user-provided buffer sequences

The `frame` interface receives the user-provided buffer sequence and in the same call frames it into the destination buffer span. Any octets that are not accepted remain with the caller, which re-offers them on the next call, so the serializer never needs to flatten the sequence into an internal array of descriptors or keep it alive between calls.

## Error handling

All errors surface from `frame`: a call yields either descriptors to write or an error, never both, so nothing framed in a failed call ever reaches the wire. When `frame` reports an error, the message is failed and serialization cannot proceed; the only valid operations are `start` and destruction. Error codes are split according to where the contract was broken:

- A body that disagrees with a size declared in the message, a `Content-Length` mismatch, or body octets supplied for a bodiless message or a HEAD response results in `error::body_size_mismatch`.

- Breaking the interface contract under chunked or close-delimited framing (for example, supplying more octets after the end of the body has been declared) results in `std::errc::invalid_argument`.

- An error returned by the encoder propagates as-is, and subsequent calls on the failed message return `std::errc::state_not_recoverable`.

Once the message completes, supplying further body octets whether through `frame` or staged through `prepare` and `commit` results in `std::errc::invalid_argument` regardless of the framing. Calling `frame` on a completed message with nothing to supply is a harmless no-op that returns no descriptors. This is how a write loop discovers completion, analogous to a read returning zero at end of stream.

## Cancellation and resumption

The serializer's accounting is designed to remain faithful to the wire when an operation is cancelled mid-write. Completion counts already reported by the stream cover exactly the consumed octets, `consume` translates them into accepted body octets, and no per-operation counters exist that could be lost with a cancelled coroutine frame. A caller resumes an interrupted message either by re-offering the unconsumed remainder of its buffers or by committing that remainder into the staging buffer and draining it; both approaches continue the message from precisely where the wire stopped.

## Quality of the implementation

### One gathered write covers the whole message

`frame` returns a single span of descriptors containing the remaining header octets, the current chunk prefix merged with the staged octets, the supplied body by reference, the chunk epilogue, and the trailer headers. The chunk prefix is written backwards into a small margin directly ahead of the staging region, so the prefix and the staged data form one contiguous descriptor instead of two. A small message therefore goes out in a single `write_some`, and a large zero-copy body adds exactly one gathered write per buffer.

## The `message_writer` interface

```cpp
template<capy::WriteStream S>
class message_writer
{
public:
    /// Constructor; the stream and serializer
    /// must outlive the writer.
    message_writer(S* stream, serializer* sr) noexcept;

    /// Return writable staging memory.
    std::span<capy::mutable_buffer>
    prepare(std::span<capy::mutable_buffer> dest);

    /// Commit staged octets, flushing the
    /// staging buffer when it runs low.
    capy::io_task<>
    commit(std::size_t n);

    /// Commit final octets and end the body.
    capy::io_task<>
    commit_eof(std::size_t n);

    /// End the body with no more octets.
    capy::io_task<>
    write_eof();

    /// Write the header and any staged data,
    /// without ending the body.
    capy::io_task<>
    write_header();

    /// Write at least one octet of `buffers`;
    /// small inputs are coalesced without I/O.
    template<capy::ConstBufferSequence CB>
    capy::io_task<std::size_t>
    write_some(CB buffers);

    /// Write until `buffers` is fully consumed.
    template<capy::ConstBufferSequence CB>
    capy::io_task<std::size_t>
    write(CB buffers);

    /// Write final octets and end the body.
    template<capy::ConstBufferSequence CB>
    capy::io_task<std::size_t>
    write_eof(CB buffers);
};
```

The rationale for `message_writer` is to satisfy the `capy::WriteStream`, `capy::WriteSink` and `capy::BufferSink` concepts, so it composes with generic stream algorithms.

`message_writer::write_header` flushes pending output without ending the body. This is the building block for `Expect: 100-continue` support in higher-level libraries, where the server must receive the header before the body is generated. Once it returns, the framing and encoding are fixed, and the late-framing rewrites no longer apply.

## Related links

Reference documentation for the Burl implementation:

- [`serializer`](https://develop.burl.cpp.al/burl/reference/boost/burl/serializer.html)
- [`message_writer`](https://develop.burl.cpp.al/burl/reference/boost/burl/message_writer.html)
