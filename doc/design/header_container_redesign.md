# Header container redesign

## Summary

This document describes a redesign of the library's header containers. Like the existing design, the new design continues to use a single flat buffer holding the serialized form of the head section and a lookup table of entries growing downward from the buffer's tail. This makes serialization essentially free because the head section is always in serialized form, and enables in-place parsing because the received head section can be indexed directly in the tail lookup table without requiring extra copies. Everything around that core changes. Per-object state shrinks by an order of magnitude, message-framing semantics move out of the field-storage layer into a dedicated head layer, and header parsing becomes a public, reusable component.

The design is not speculative: it is implemented and tested in the Burl project (<https://github.com/cppalliance/burl>).

## The 256-byte metadata blob is gone

Every container currently embeds `detail::header`, which embeds `metadata`: six per-field records, each carrying a 24-byte `error_code`, a `size_t` occurrence count, and the parsed value, totaling 256 bytes. This cost is paid even by a plain `fields`, which can never be asked a framing question.

The new design removes this state from `fields_base` and uses 16 bytes of state in the upper-layer `message_head_base`. This provides the same functionality while also allowing the parser to use the same state during parsing. Thus, the state naturally falls out of a parsed message without any post-processing. The rest of this document explains each change and the rationale behind it in more detail.

The sizes below are measured from that implementation on x86-64:

|                 | current | new  |               |
| --------------- | ------- | ---- | ------------- |
| `fields`        | 336 B   | 32 B | 10.5× smaller |
| `request_head`  | 336 B   | 48 B | 7× smaller    |
| `response_head` | 336 B   | 48 B | 7× smaller    |

## Framing state is a 16-byte cache on the head containers

`message_head_base` adds exactly three things to `fields_base`: the Content-Length value (`uint64_t`), a request/response start-line union, and flag bits. The framing observers (`payload()`, `content_length()`, `chunked()`, `keep_alive()`, `upgrade()`, `version()`) perform O(1) reads of this state.

The state is updated and kept in sync with field modifications using `fields_base::on_special_(field)` when one of the five framing-relevant names changes. The head then re-derives only that field's bits by walking its occurrences through the ID-indexed table.

## Parsed and hand-built heads share one cache

`head_parser` populates and consults the same 16-byte cache on the head containers while parsing. Each arriving field is classified once, and because the flags summarize everything seen so far, malformed frames are rejected at the offending field without reiterating the header.

The result of parsing is a `message_head_base` built over the received bytes. Thus, a parsed head and a hand-built head are the same type, with the same observers backed by the same cache.

## `fields_base` no longer knows what an HTTP message is

The new `fields_base` is generic field storage over the wire form. Its entire upward interface consists of three virtuals (`static_`, `on_clear_`, `on_special_`), and everything HTTP/1-specific lives in `message_head_base`.

This moves the complexity of message integrity and framing to `message_head_base` and makes the implementation and testing of the `fields_base` simpler.

## Lookup-table entry field offset calculation

The new entry struct uses `uint16_t` for the lengths of the field name and value, which reduces each entry's size from 20 bytes to 12 bytes. This change is based on the assumption that there is no practical use for `uint32_t` here; multiple HTTP libraries and proxies in the wild already have the same hard limit, or even smaller limits. (Note that the entire header section still uses `uint32_t` for the offset and can have a maximum size of 2^32 − 1.)

The smaller index table not only reduces the overhead of the container but also makes scanning the container for a field ID faster, because more entries fit into a single cache line.

The base pointer now addresses the field section itself. `buf` points past the start line, which can be recovered as `buf - prefix`. As a result, forming a field name or value pointer requires a single addition, `buf + field_offset`, with no need to involve the `prefix` value.

```CPP
    +------------+--------+------+-----------------------------+
    | start line | fields | free | entry[count-1] ... entry[0] |
    +------------+--------+------+-----------------------------+
    ^            ^        ^                                    ^
    |            buf      buf + size                         end
    buf - prefix
```

## `head_parser`: the library's header parsing, made public

The new design adds `head_parser`, an incremental, in-place, allocation-free parser over a caller-owned buffer. This is a lower-level facility that allows users to build their own parser on top of it.

The library's `parser` embeds this same object, so a custom parser built on `head_parser` gets identical syntax, limits, and framing behavior to the library's own parser.

If this facility proves to be of little use, it can be moved to `detail` and used only by library's own parser, reducing the complexity exposed by header parsing to the rest of the parser.

## Naming: the `_head` suffix

The containers hold a message head (the start line and the field section). Calling them `request`/`response` claims more than they store, and it also takes the names that a higher layer naturally wants most: a client or a server naturally spells its complete-message types `request` and `response`.

| Old            | New                 |
| -------------- | ------------------- |
| `request`      | `request_head`      |
| `response`     | `response_head`     |
| `message_base` | `message_head_base` |

## Error model and validation

In the new design, field and head container modifiers store names and values verbatim. The idea is to move character-set validation to the serializer because the head is one contiguous buffer, the serializer can therefore validate the entire section in a single forward pass when the message is actually sent.

Because this is a single, well-defined step, it can be made a serializer configuration option, with validation enabled by default. For most applications, field values come from trusted internal sources or have already been validated at the boundary where untrusted input entered the system. Re-validating them on every insertion or modification would add overhead without providing additional safety.

This also provides a no-throw guarantee for server implementations as possible non-conformance is reported through the serializer's `error_code` channel instead of throwing from request handlers when fields are inserted or modified.

## Related links

Reference documentation for the Burl implementation:

- [`fields_base`](https://develop.burl.cpp.al/burl/reference/boost/burl/fields_base.html)
- [`fields`](https://develop.burl.cpp.al/burl/reference/boost/burl/fields.html)
- [`message_head_base`](https://develop.burl.cpp.al/burl/reference/boost/burl/message_head_base.html)
- [`request_head`](https://develop.burl.cpp.al/burl/reference/boost/burl/request_head.html)
- [`response_head`](https://develop.burl.cpp.al/burl/reference/boost/burl/response_head.html)
- [`head_parser`](https://develop.burl.cpp.al/burl/reference/boost/burl/head_parser.html)
