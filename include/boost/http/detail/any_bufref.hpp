//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_ANY_BUFREF_HPP
#define BOOST_HTTP_DETAIL_ANY_BUFREF_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/buffers.hpp>
#include <cstddef>

namespace boost {
namespace http {
namespace detail {

/** A type-erased const buffer sequence reference.

    This class provides a type-erased interface for iterating
    over buffer sequences without knowing the concrete type.
*/
class any_bufref
{
public:
    /** Construct from a const buffer sequence.

        @param bs The buffer sequence to adapt.
    */
    template<capy::ConstBufferSequence BS>
    explicit
    any_bufref(BS const& bs) noexcept
        : bs_(&bs)
        , fn_(&copy_impl<BS>)
    {
    }

    /** Fill an array with buffers from the sequence.

        @param dest Pointer to array of mutable buffer descriptors.

        @param n Maximum number of buffers to copy.

        @return The number of buffers actually copied.
    */
    std::size_t
    copy_to(
        capy::mutable_buffer* dest,
        std::size_t n) const noexcept
    {
        return fn_(bs_, dest, n);
    }

private:
    template<capy::ConstBufferSequence BS>
    static std::size_t
    copy_impl(
        void const* p,
        capy::mutable_buffer* dest,
        std::size_t n)
    {
        auto const& bs = *static_cast<BS const*>(p);
        auto it = capy::begin(bs);
        auto const end_it = capy::end(bs);

        std::size_t i = 0;
        for(; it != end_it && i < n; ++it, ++i)
        {
            auto const& buf = *it;
            dest[i] = capy::mutable_buffer(
                const_cast<char*>(
                    static_cast<char const*>(buf.data())),
                buf.size());
        }
        return i;
    }

    using fn_t = std::size_t(*)(void const*,
        capy::mutable_buffer*, std::size_t);

    void const* bs_;
    fn_t fn_;
};

} // detail
} // http
} // boost

#endif
