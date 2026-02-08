//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_REQUEST_PARSER_HPP
#define BOOST_HTTP_REQUEST_PARSER_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/error.hpp>
#include <boost/http/method.hpp>
#include <boost/http/parser.hpp>
#include <boost/http/static_request.hpp>

#include <memory>

namespace boost {
namespace http {

/// @copydoc parser
/// @brief A parser for HTTP/1 requests.
/// @see @ref response_parser.
class request_parser
    : public parser
{
public:
    /** Destructor.

        Any views or buffers obtained from this
        parser become invalid.
    */
    ~request_parser() = default;

    /** Default constructor.

        Constructs a parser with no allocated state.
        The parser must be assigned from a valid
        parser before use.

        @par Postconditions
        The parser has no allocated state.
    */
    request_parser() = default;

    /** Constructor.

        Constructs a parser with the provided configuration.

        The parser will allocate the required space on
        startup based on the config parameters, and will
        not perform any further allocations.

        @par Example
        @code
        auto cfg = make_parser_config(parser_config{true});
        request_parser pr(cfg);
        @endcode

        @par Complexity
        Constant.

        @par Exception Safety
        Calls to allocate may throw.

        @param cfg Shared pointer to parser configuration.

        @see @ref make_parser_config, @ref parser_config.
    */
    BOOST_HTTP_DECL
    explicit
    request_parser(
        std::shared_ptr<parser_config_impl const> cfg);

    /** Constructor.

        The states of `other` are transferred
        to the newly constructed object,
        including the allocated buffer.
        After construction, the only valid
        operations on the moved-from object
        are destruction and assignment.

        Buffer sequences previously obtained
        using @ref prepare or @ref pull_body
        remain valid.

        @par Complexity
        Constant.

        @param other The parser to move from.
    */
    request_parser(
        request_parser&& other) noexcept = default;

    /** Assignment.
        The states of `other` are transferred
        to this object, including the allocated
        buffer.
        After assignment, the only valid
        operations on the moved-from object
        are destruction and assignment.
        Buffer sequences previously obtained
        using @ref prepare or @ref pull_body
        remain valid.
        @par Complexity
        Constant.
        @param other The parser to move from.
    */
    request_parser&
    operator=(request_parser&& other) noexcept
    {
        assign(std::move(other));
        return *this;
    }

    /** Return a reference to the parsed request headers.

        The returned reference remains valid until:
        @li @ref start is called
        @li @ref reset is called
        @li The parser instance is destroyed

        @par Preconditions
        @code
        this->got_header() == true
        @endcode

        @par Exception Safety
        Strong guarantee.

        @see
            @ref got_header.
    */
    BOOST_HTTP_DECL
    static_request const&
    get() const;
};

} // http
} // boost

#endif
