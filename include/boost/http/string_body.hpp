//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_STRING_BODY_HPP
#define BOOST_HTTP_STRING_BODY_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/buffers.hpp>
#include <string>
#include <utility>

namespace boost {
namespace http {

/** A ConstBufferSequence adapter for an owned `std::string`.

    Takes ownership of a `std::string` and exposes
    it via an interface conforming to the
    ConstBufferSequence requirements.

    @par Example
    @code
    serializer sr(ctx);
    response res(status::not_found);
    std::string body =
        "<html>\n"
        "    <body>\n"
        "        <h1>404 Not Found</h1>\n"
        "        <p>Sorry, the page does not exist.</p>\n"
        "    </body>\n"
        "</html>\n";
    res.set_payload_size(body.size());
    sr.start<string_body>(res, std::move(body));
    @endcode

    @see
        @ref serializer.
*/
class string_body
{
    std::string s_;
    capy::const_buffer cb_;

public:
    /// The type for each buffer.
    using value_type = capy::const_buffer;

    /// The type of a const iterator.
    using const_iterator = capy::const_buffer const*;

    string_body(
        string_body&& other) noexcept
        : s_(std::move(other.s_))
        , cb_(s_.data(), s_.size())
    {
        other.cb_ = {};
    }

    /** Constructor.
    */
    string_body(
        string_body const& other) = delete;

    /** Constructor.

        @param s The string to take ownership of.
    */
    string_body(
        std::string s) noexcept
        : s_(std::move(s))
        , cb_(s_.data(), s_.size())
    {
    }

    /** Return an iterator to the beginning of the
        buffer sequence.
    */
    const_iterator
    begin() const noexcept
    {
        return &cb_;
    }

    /** Return an iterator to the end of the
        buffer sequence.
    */
    const_iterator
    end() const noexcept
    {
        return &cb_ + 1;
    }
};

} // http
} // boost

#endif
