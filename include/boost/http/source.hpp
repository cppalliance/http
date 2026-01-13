//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SOURCE_HPP
#define BOOST_HTTP_SOURCE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/buffers.hpp>
#include <boost/core/span.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <type_traits>

namespace boost {
namespace http {

/** An interface for producing buffers of data.

    This interface abstracts the production of
    a finite stream of data, returned by writing
    into caller-provided buffers until there
    is no more output data.

    @par Thread Safety
    Non-const member functions may not be
    called concurrently on the same instance.

    @see
        @ref file_source,
        @ref sink,
        @ref serializer.
*/
struct BOOST_HTTP_SYMBOL_VISIBLE
    source
{
    /** The results of producing data.
    */
    struct results
    {
        /** The error, if any occurred.
        */
        system::error_code ec;

        /** The number of bytes produced in the output.
        */
        std::size_t bytes = 0;

        /** True if there will be no more output.
        */
        bool finished = false;

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
            std::size_t bytes_,
            bool finished_) noexcept
            : ec(ec_)
            , bytes(bytes_)
            , finished(finished_)
        {
        }
    #endif
    };

    /** Produce data.

        This function attempts to read from the
        source, placing the data into the given
        mutable buffer sequence.
        The return value indicates the number of
        bytes placed into the buffers, the error
        if any occurred, and a `bool` indicating
        whether or not there is more data
        remaining in the source.

        @par Preconditions
        @code
        buffer_size(bs) != 0
        @endcode

        @par Postconditions
        @code
        rv.ec.failed() == true || rv.finished == true || rv.bytes == buffer_size(bs)
        @endcode

        @return The result of the operation.

        @param bs The buffers to use.
        Each buffer in the sequence will
        be filled completely before data
        is placed in the next buffer.
    */
    template<class MutableBufferSequence>
    results
    read(MutableBufferSequence const& bs)
    {
        static_assert(
            capy::mutable_buffer_sequence<MutableBufferSequence>,
            "Type requirements not met");

        return read_impl(bs);
    }

protected:
    /** Derived class override.

        This pure virtual function is called by
        the implementation and must be overriden.
        The callee should attempt to place data
        into the given mutable buffer.
        The return value must be set to indicate
        the number of bytes placed into the
        buffers, the error if any occurred,
        and a `bool` indicating whether or
        not there is more data remaining
        in the source.
        The implementation must fill the
        provided buffer completely, report
        an error or set `rv.finished` to true.

        @par Preconditions
        @code
        buffer_size(bs) != 0
        @endcode

        @par Postconditions
        @code
        rv.ec.failed() == true || rv.finished == true || rv.bytes == buffer_size(bs)
        @endcode

        @return The result of the operation.

        @param b The buffer to use.
        If this is not filled completely,
        then the result must indicate failure
        or that no more data remains (or both).
    */
    virtual
    results
    on_read(
        capy::mutable_buffer b) = 0;

    /** Derived class override.

        This pure virtual function is called by
        the implementation and must be overriden.
        The callee should attempt to place data
        into the given mutable buffer sequence.
        The return value must be set to indicate
        the number of bytes placed into the
        buffers, the error if any occurred,
        and a `bool` indicating whether or
        not there is more data remaining
        in the source.

        @par Preconditions
        @code
        buffer_size(bs) != 0
        @endcode

        @par Postconditions
        @code
        rv.ec.failed() == true || rv.finished == true || rv.bytes == buffer_size(bs)
        @endcode

        @return The result of the operation.

        @param bs The buffer sequence to use.
        Each buffer in the sequence must
        be filled completely before data
        is placed in the next buffer.
        If the buffers are not filled
        completely, then the result must
        indicate failure or that no more
        data remains (or both).
    */
    BOOST_HTTP_DECL
    virtual
    results
    on_read(
        boost::span<capy::mutable_buffer const> bs);

private:
    results
    read_impl(
        capy::mutable_buffer const& b)
    {
        return on_read(b);
    }

    results
    read_impl(
        boost::span<capy::mutable_buffer const> const& bs)
    {
        return on_read(bs);
    }

    template<class T>
    results
    read_impl(T const&);
};

//------------------------------------------------

/** A type trait that determines if T is a source.

    @tparam T The type to check.

    @see
        @ref source.
*/
template<class T>
using is_source =
    std::is_convertible<
        typename std::decay<T>::type*,
        source*>;

} // http
} // boost

#include <boost/http/impl/source.hpp>

#endif
