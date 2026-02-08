[![Boost.HTTP](doc/modules/ROOT/images/repo-logo.png)](http://master.http.cpp.al/)

| | Docs | GitHub Actions | Drone | Codecov |
|---|---|---|---|---|
| [`master`](https://github.com/cppalliance/http/tree/master) | [![Documentation](https://img.shields.io/badge/docs-master-brightgreen.svg)](https://master.http.cpp.al/) | [![CI](https://github.com/cppalliance/http/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/cppalliance/http/actions/workflows/ci.yml?query=branch%3Amaster) | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/http/status.svg?ref=refs/heads/master)](https://drone.cpp.al/cppalliance/http/branches) | [![codecov](https://codecov.io/gh/cppalliance/http/branch/master/graph/badge.svg)](https://app.codecov.io/gh/cppalliance/http/tree/master) |
| [`develop`](https://github.com/cppalliance/http/tree/develop) | [![Documentation](https://img.shields.io/badge/docs-develop-brightgreen.svg)](https://develop.http.cpp.al/) | [![CI](https://github.com/cppalliance/http/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/cppalliance/http/actions/workflows/ci.yml?query=branch%3Adevelop) | [![Build Status](https://drone.cpp.al/api/badges/cppalliance/http/status.svg?ref=refs/heads/develop)](https://drone.cpp.al/cppalliance/http/branches) | [![codecov](https://codecov.io/gh/cppalliance/http/branch/develop/graph/badge.svg)](https://app.codecov.io/gh/cppalliance/http/tree/develop) |

## Boost.HTTP

### This is currently **NOT** an official Boost library.

### Overview

HTTP powers the web, but implementing it correctly in C++ is surprisingly
hard. Boost.HTTP gives you the entire HTTP/1.1 protocol stack — from raw
parsing up to complete clients and servers — without tying you to any
network library.

The library follows a **Sans-I/O** architecture at every layer. Even
high-level clients and servers operate on abstract, type-erased stream
interfaces rather than concrete sockets. There is no dependency on any
networking or I/O library — not even in the implementation files. You
bring your own transport.

The execution model is **coroutines-only**, built on
[Boost.Capy](https://github.com/cppalliance/capy), a C++20 coroutine
I/O foundation that provides task types, buffer sequences, stream
concepts, and type-erased stream wrappers.

### Features

* Multiple levels of abstraction — low-level parsing and serialization
  through complete clients and servers
* Sans-I/O from top to bottom — works with any I/O framework (Asio,
  io_uring, platform APIs)
* Coroutines-only execution model built on Capy
* Type-erased streams reflecting buffer-oriented I/O
* Express.js-style router with pattern matching and middleware
  composition
* High-level components: static file serving, form processing, cookie
  handling, cryptographic utilities
* Content encodings (gzip, deflate, brotli)
* Strict RFC 9110 compliance to prevent common security issues
* Fast compilation, minimal template exposure

### Quick Example

```cpp
#include <boost/http.hpp>

using namespace boost::http;

int main()
{
    // Build a request
    request req(method::get, "/api/users");
    req.set(field::host, "example.com");
    req.set(field::accept, "application/json");

    // Build a response
    response res(status::ok);
    res.set(field::content_type, "application/json");
    res.set(field::content_length, "42");

    // Access the serialized form
    std::cout << req.buffer();
    std::cout << res.buffer();
}
```

Output:

```
GET /api/users HTTP/1.1
Host: example.com
Accept: application/json

HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 42

```

### Design

The Sans-I/O separation yields concrete benefits:

**Reusability.** The same HTTP code works with any transport. Write the
protocol logic once, plug in any I/O framework.

**Testability.** Tests run as pure function calls without sockets,
timers, or network delays. Coverage is higher, execution is faster,
results are deterministic.

**Security.** The parser is strict by default. Malformed input that
could enable request smuggling or header injection is rejected
immediately.

### Requirements

* C++20 compiler with coroutine support
* Boost libraries (core, system)
* Link to the static or dynamic library

### Supported Compilers

* GCC: 11 to 14
* Clang: 14 to 18
* MSVC: 14.3 to 14.42

### License

Distributed under the Boost Software License, Version 1.0.
(See accompanying file [LICENSE_1_0.txt](LICENSE_1_0.txt) or copy at
https://www.boost.org/LICENSE_1_0.txt)
