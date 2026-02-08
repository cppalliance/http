# Documentation Outline

## 1. index.adoc — Introduction

- Title + problem statement
- What This Library Does
  - HTTP at multiple abstraction levels (low-level to complete clients/servers)
  - Sans-I/O top to bottom — no dependency on Corosio, Asio, or any transport
  - Coroutines-only execution model (Capy-based)
  - Type-erased streams reflecting buffer-oriented I/O
  - High-level components: file serving, forms, cookies, cryptography
- What This Library Does Not Do
- Target Audience
- Design Philosophy
- Requirements
- Code Conventions
- Quick Example
- Next Steps

Existing page: `index.adoc` — rewrite in place

## 2. 2.http-tutorial.adoc — HTTP Tutorial

- Educate the reader on the HTTP protocol itself
- Sessions, request/response model, message structure
- Methods, status codes, headers
- Security considerations

Renamed from: `http-protocol.adoc`

## 3. HTTP Messages

### 3. 3.messages.adoc — HTTP Messages (intro)

- Landing page for the section
- How containers, serializing, and parsing relate
- Overview of the data model and the two-sided stream architecture

New page (no existing equivalent)

### 3a. 3a.containers.adoc — Containers

- `request`, `response`, `fields`
- Methods, status codes, reason strings
- Building and inspecting messages

Renamed from: `containers.adoc`

### 3b. 3b.serializing.adoc — Serializing

- How the serializer works: persistent object tied to the session/socket lifetime
- Two sides: input (sink — accepts message and body) and output (stream — emits serialized HTTP)
- General concepts first, then the input side (sink interface), then the output side (stream interface)
- Chunked encoding, content encoding
- `Expect: 100-continue` handshake

Renamed from: `serializing.adoc`

### 3c. 3c.parsing.adoc — Parsing

- How the parser works: persistent object tied to the session/socket lifetime
- Two sides: input (stream — caller provides a read stream the parser draws from) and output (source — parsed message and body)
- General concepts first, then the input side (stream interface), then the output side (source interface)
- `request_parser`, `response_parser`
- Incremental parsing, limits, error handling

Renamed from: `parsing.adoc`

## 4. HTTP Servers

### 4. 4.servers.adoc — HTTP Servers (intro)

- Landing page for the section
- Overview of the server architecture
- How the pieces fit together: worker, route handlers, routers, middleware

Renamed from: `server/servers-intro.adoc`

### 4a. 4a.http-worker.adoc — HTTP Worker

- The core server loop
- Connection management, session lifecycle

New page (no existing equivalent)

### 4b. 4b.route-handlers.adoc — Route Handlers

- Smallest unit of server logic
- Handler signature, communicating with the server
- Middleware concepts as they apply to handlers

Renamed from: `server/route-handlers.adoc`

### 4c. 4c.routers.adoc — Routers

- Express.js-style request dispatch
- Nested routers
- Middleware composition within routers

Renamed from: `server/routers.adoc` (also merges `router.adoc`)

### 4d. 4d.route-patterns.adoc — Route Patterns

- Pattern syntax: literals, named parameters, wildcards, optional groups
- Escaping special characters, quoted parameter names
- Grammar reference
- Matching behavior, router options
- Pattern examples and error cases

Renamed from: `server/route-patterns.adoc`

### 4e. 4e.serve-static.adoc — Serving Static Files

- Static file delivery
- Content types, caching, conditional requests, range requests

Renamed from: `server/serve-static.adoc`

### 4f. 4f.serve-index.adoc — Directory Listings

- Browsable directory listings

Renamed from: `server/serve-index.adoc`

### 4g. 4g.bcrypt.adoc — BCrypt Password Hashing

- Secure password hashing and verification

Renamed from: `bcrypt.adoc`

## 5. Compression

### 5. 5.compression.adoc — Compression (intro)

- Landing page for the section
- How compression is provided as individual separate libraries

New page (no existing equivalent)

### 5a. 5a.zlib.adoc — ZLib

- DEFLATE, zlib format, gzip

Renamed from: `compression/zlib.adoc`

### 5b. 5b.brotli.adoc — Brotli

- High-ratio compression

Renamed from: `compression/brotli.adoc`

## 6. Design

### 6a. 6a.sans-io.adoc — Sans-I/O Philosophy

- What Sans-I/O is and why it matters
- Reusability, testability, determinism
- Comparison with I/O-coupled designs

Renamed from: `sans_io_philosophy.adoc`

### 6b. 6b.parser.adoc — Parser

- Comparison to Boost.Beast parser design
- Memory allocation and utilization
- Input buffer preparation, two-phase parsing
- Use cases and interfaces

Renamed from: `design_requirements/parser.adoc`

### 6c. 6c.serializer.adoc — Serializer

- Use cases and interfaces
- Empty body, WriteSink body, BufferSink body

Renamed from: `design_requirements/serializer.adoc`

## 7. 7.reference.adoc — API Reference

Existing page: `reference.adoc` — renamed

---

## Unmapped Existing Pages

These pages do not map to a section in the new outline:

- **`router.adoc`** — Standalone router page. Material merged into Routers (4c).
