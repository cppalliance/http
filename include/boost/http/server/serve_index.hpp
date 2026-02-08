//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_SERVE_INDEX_HPP
#define BOOST_HTTP_SERVER_SERVE_INDEX_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/route_handler.hpp>

namespace boost {
namespace http {

/** Coroutine-based directory listing middleware.

    This middleware generates directory listings for
    requests that map to filesystem directories. It
    supports content negotiation, returning HTML, JSON,
    or plain text responses based on the Accept header.

    This handler is intended to complement @ref serve_static.
    When `serve_static` encounters a directory without an
    index file, `serve_index` can provide a browsable listing.

    @par Example
    @code
    router r;
    r.use( serve_static( root ) );
    r.use( serve_index( root ) );
    @endcode

    @see serve_static
*/
class BOOST_HTTP_DECL serve_index
{
    struct impl;
    impl* impl_;

public:
    /** Options for directory listing.
    */
    struct options
    {
        /// Show hidden files (dotfiles). Default: false.
        bool hidden = false;

        /// Show parent directory ("..") link. Default: true.
        bool show_parent = true;

        /// Treat non-GET/HEAD as unhandled (true) or 405 (false).
        bool fallthrough = true;
    };

    /** Destructor.
    */
    ~serve_index();

    /** Construct with document root and default options.

        @param root The document root path.
    */
    explicit serve_index(core::string_view root);

    /** Construct with document root and options.

        @param root The document root path.

        @param opts Configuration options.
    */
    serve_index(
        core::string_view root,
        options const& opts);

    /** Move constructor.
    */
    serve_index(serve_index&& other) noexcept;

    /** Handle a request.

        Lists the contents of the directory matching the
        request path. Uses the Accept header to choose
        between HTML, JSON, and plain text responses.

        @param rp The route parameters.

        @return A task that completes with the routing result.
    */
    route_task operator()(route_params& rp) const;
};

} // http
} // boost

#endif
