# CLAUDE.md

## Code Style

- C++11 unless otherwise specified
- Boost C++ Libraries naming conventions (snake_case)
- 4-space indentation, no tabs
- Braces on their own line for classes/functions
- Symbols in "detail" namespaces are never public
- public headers in "include/"
- library cpp files in "src/"
- no ABI compatibility guarantee in different Boost version

## Javadoc Documentation

Follow Boost C++ Libraries Javadoc style:

- Brief descriptions on first line after `/**`
- Functions returning values: brief starts with "Return"
- Use `@param` for function parameters
- Use `@tparam` for template parameters, except:
  - Variadic args (`Args...`) — omit
  - Types deduced from function parameters — omit (self-evident from `@param`)
- Use `@return` for return value details
- Use `@pre` for preconditions
- Use `@post` for postconditions
- Use `@throws` for exceptions
- Use `@note` for important notes
- Use `@see` for cross-references
- Use `@code` / `@endcode` for examples

## Examples

```cpp
/** Return the size of the buffer sequence.

    @param buffers The buffer sequence to measure.

    @return The total byte count.
*/
template<class BufferSequence>
std::size_t
buffer_size(BufferSequence const& buffers);
```

No `@tparam` needed—`BufferSequence` is evident from `@param buffers`.

```cpp
/** Return the default value.

    @tparam T The value type.
*/
template<class T>
T default_value();
```

`@tparam` needed—`T` has no corresponding function parameter.

## Preferences

- Concise, dry answers
- Full files, not diffs
- Accurate, compiling C++ code

# Project

## Structure

* Boost.Beast2 refers to both the library which uses Boost.Asio and the collection of
  libraries which includes Boost.Capy, Boost.Buffers, Boost.Http (http-proto), and Boost.Beast2

* Boost.Capy contains general purpose data structures and algorithms

* Boost.Buffers provides Asio-style buffers without requiring Asio, and it
  introduces blocking source and sink types with buffered variants

* Boost.Http provides a Sans-IO HTTP which includes the following:
  - Containers for request and response headers
  - Low level HTTP algorithms such as parsing, serialization, and field processing
  - High level HTTP Server components
  - High level HTTP Client components

* Boost.Beast2 builds on Capy, Buffers, and Boost.Http by providing the I/O layer
  using Boost.Asio. It can open ports and maintain TCP connections.

* Boost.Http Server component is based on an Express JS style router, ported to
  idiomatic C++

  - In the C++ router, route handlers are declarative rather than imperative. They
    package up the response using a Sans-IO API. Beast2 handles the sesnding
    and receiving.

  - Alternatively, a route handler can opt-in to seeing the Asio stream type
    and its associated executor, and bypass the declarative framework to take
    over the async aspect of sending and receiving the bytes.

  - Sans-IO route handlers can be built depending only on Boost.Http (no Asio)
    while opting-in to seeing the Asio stream allows for C++20 style co_await
    and all the other async continuation models that Asio supports.

# Documentation

## System Context

You are an expert programming educator creating tutorial documentation for expert C++ programmers.
Goal: Minimize time-to-competence. Maximize learning velocity.
Style: Problem-first, progressive disclosure, concrete examples.

## Instructions

INPUT ANALYSIS
- Identify distinct problems this API solves
- Map API concepts to problem domains
- Determine prerequisite knowledge (assume expert C++, standard dependencies)

CHAPTER DESIGN
- One problem domain per chapter
- Order by: simplest useful → most powerful
- Each chapter states upfront: "After this you can [solve X problem]"
- Build dependency chain: later chapters assume earlier ones

CHAPTER STRUCTURE
1. Problem hook: concrete scenario reader wants to solve
2. Minimal working solution: fastest path to success
3. Progressive enhancement: add one capability at a time
4. Each step: why this matters, what it unlocks
5. End state: reader can now [do concrete thing]

OPTIMIZATION PRINCIPLES
- No teaching C++ basics or documented dependencies
- Show power immediately (avoid toy examples)
- Concrete > abstract (real problems > hypotheticals)
- Remove anything not on critical path to competence
- Examples must compile and demonstrate capability
- Emotional tone: "you can do powerful things quickly"

TRANSPARENCY
- State chapter scope and outcomes first
- Flag prerequisites explicitly
- No surprise complexity or hidden dependencies

VALIDATION METRIC
- Could reader A/B test learn faster from this than alternatives?
- Does each paragraph reduce time-to-competence?
