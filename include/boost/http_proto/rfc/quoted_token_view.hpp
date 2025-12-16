//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_RFC_QUOTED_TOKEN_VIEW_HPP
#define BOOST_HTTP_PROTO_RFC_QUOTED_TOKEN_VIEW_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/url/grammar/string_view_base.hpp>
#include <boost/core/detail/string_view.hpp>

namespace boost {
namespace http_proto {

namespace implementation_defined {
struct quoted_token_rule_t;
} // implementation_defined

/** A view into a quoted string token, which may
    contain escape sequences.

    @see
        @ref quoted_token_view.
*/
class quoted_token_view final
    : public grammar::string_view_base
{
    std::size_t n_ = 0;

    friend struct implementation_defined::quoted_token_rule_t;

    // unquoted
    explicit
    quoted_token_view(
        core::string_view s) noexcept
        : string_view_base(s)
        , n_(s.size())
    {
    }

    // maybe quoted
    quoted_token_view(
        core::string_view s,
        std::size_t n) noexcept
        : string_view_base(s)
        , n_(n)
    {
    }

public:
    quoted_token_view() = default;

    quoted_token_view(
        quoted_token_view const&) noexcept = default;

    quoted_token_view& operator=(
        quoted_token_view const&) noexcept = default;

    /** Return true if the token contains escape
        sequences.

        @par Complexity
        Constant.

        @Return true if the token contains
        escape sequences.
    */
    bool
    has_escapes() const noexcept
    {
        return n_ != s_.size();
    }

    /** Return the size of the unescaped content.

        @par Complexity
        Constant.

         @return Size of the unescaped string content.
    */
    std::size_t
    unescaped_size() const noexcept
    {
        return n_;
    }
};

} // http_proto
} // boost

#endif
