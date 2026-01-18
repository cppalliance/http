//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SINK_HPP
#define BOOST_HTTP_SINK_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/core/span.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <type_traits>

namespace boost {
namespace http {

/** An interface for consuming buffers of data.

    This interface abstracts the consumption of
    a finite stream of data, passed by reading
    from caller-provided buffers until there
    is no more input data.

    @par Thread Safety
    Non-const member functions may not be
    called concurrently on the same instance.

    @see
        @ref file_sink,
        @ref source,
        @ref parser.
*/
struct BOOST_HTTP_SYMBOL_VISIBLE
    sink
{
    /** The results of consuming data.
    */
    struct results
    {
        /** The error, if any occurred.
        */
        system::error_code ec;

        /** The number of bytes consumed in the input.
        */
        std::size_t bytes = 0;

        /** Accumulate results.
        */
        results&
        operator+=(
            results const& rv) noexcept;
        
    #ifdef BOOST_HTTP_AGGREGATE_WORKAROUND
        constexpr
        results() = default;

        constexpr
        results(
            system::error_code ec_,
            std::size_t bytes_) noexcept
            : ec(ec_)
            , bytes(bytes_)
        {
        }
    #endif
    };

    /** Consume data.

        This function attempts to write to the
        sink, by transferring data from the given
        constant buffer sequence.
        The return value indicates the number of
        bytes consumed from the buffers and the
        error if any occurred.

        @par Preconditions
        @li This is the first call to `write`, or
        the last value of `more` was `true`.
        @li buffer_size(bs) != 0

        @par Postconditions
        @code
        rv.ec.failed() == true || rv.bytes == buffer_size(bs)
        @endcode

        @return The result of the operation.

        @param bs The buffers to use.
        Each buffer in the sequence will be
        consumed completely before the next
        buffer is accessed.

        @param more `true` if there will be one
        or more subsequent calls to @ref write.
    */
    template<class ConstBufferSequence>
    results
    write(
        ConstBufferSequence const& bs,
        bool more)
    {
        static_assert(
            capy::ConstBufferSequence<ConstBufferSequence>,
            "Type requirements not met");

        return write_impl(bs, more);
    }

protected:
    /** Derived class override.

        This pure virtual function is called by
        the implementation and must be overriden.
        The callee should attempt to consume data
        from the given constant buffer.
        The return value must be set to indicate
        the number of bytes consumed from the
        buffers, and the error if any occurred.

        @par Preconditions
        @li This is the first call to `write`, or
        the last value of `more` was `true`.
        @li buffer_size(bs) != 0

        @par Postconditions
        @code
        rv.ec.failed() == true || rv.bytes == buffer_size(bs)
        @endcode

        @return The result of the operation.

        @param b The buffer to consume.
        The result must indicate that the buffer
        was consumed completely, or that an
        error occurred.

        @param more `true` if there will be one
        or more subsequent calls.
    */
    virtual
    results
    on_write(
        capy::const_buffer b,
        bool more) = 0;

    /** Derived class override.

        This pure virtual function is called by
        the implementation and must be overriden.
        The callee should attempt to consume data
        from the given constant buffer sequence.
        The return value must be set to indicate
        the number of bytes consumed from the
        buffers, and the error if any occurred.

        @par Preconditions
        @li This is the first call to `write`, or
        the last value of `more` was `true`.
        @li
            @code
            buffer_size(bs) != 0
            @endcode

        @par Postconditions
        @code
        rv.ec.failed() == true || rv.bytes == buffer_size(bs)
        @endcode

        @return The result of the operation.

        @param bs The buffer sequence to use.
        Each buffer in the sequence must
        be completely consumed before data
        is consumed from the next buffer.
        The result must indicate that the buffer
        was consumed completely, or that an
        error occurred.

        @param more `true` if there will be one
        or more subsequent calls.
    */
    BOOST_HTTP_DECL
    virtual
    results
    on_write(
        boost::span<const capy::const_buffer> bs,
        bool more);

private:
    results
    write_impl(
        capy::const_buffer const& b,
        bool more)
    {
        return on_write(b, more);
    }

    results
    write_impl(
        capy::mutable_buffer const& b,
        bool more)
    {
        return on_write(b, more);
    }

    results
    write_impl(
        boost::span<const capy::const_buffer> const& bs,
        bool more)
    {
        return on_write(bs, more);
    }

    template<class T>
    results
    write_impl(T const&, bool);
};

//------------------------------------------------

/** A type trait that determines if T is a sink.

    @tparam T The type to check.

    @see
        @ref sink.
*/
template<class T>
using is_sink =
    std::is_convertible<
        typename std::decay<T>::type*,
        sink*>;

} // http
} // boost

#include <boost/http/impl/sink.hpp>

#endif
