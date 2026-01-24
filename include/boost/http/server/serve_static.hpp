//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_SERVE_STATIC_HPP
#define BOOST_HTTP_SERVER_SERVE_STATIC_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/router.hpp>

namespace boost {
namespace http {

/** Policy for handling dotfiles in static file serving.
*/
enum class dotfiles_policy
{
    /// Allow access to dotfiles.
    allow,

    /// Deny access to dotfiles (403 Forbidden).
    deny,

    /// Ignore dotfiles (pass to next handler).
    ignore
};

/** Options for the static file server.
*/
struct serve_static_options
{
    /// How to handle dotfiles.
    dotfiles_policy dotfiles = dotfiles_policy::ignore;

    /// Maximum cache age in seconds.
    std::uint32_t max_age = 0;

    /// Enable accepting range requests.
    bool accept_ranges = true;

    /// Enable etag header generation.
    bool etag = true;

    /// Treat client errors as unhandled requests.
    bool fallthrough = true;

    /// Enable the immutable directive in cache control headers.
    bool immutable = false;

    /// Enable a default index file for directory requests.
    bool index = true;

    /// Enable the "Last-Modified" header.
    bool last_modified = true;

    /// Enable redirection for directories missing a trailing slash.
    bool redirect = true;
};

/** Coroutine-based static file server middleware.

    This middleware serves static files from a document root
    directory. It handles Content-Type detection, caching
    headers, conditional requests, and range requests.

    @par Example
    @code
    router r;
    r.use( serve_static( "/var/www/static" ) );
    @endcode

    @see serve_static_options, dotfiles_policy
*/
class BOOST_HTTP_DECL serve_static
{
    struct impl;
    impl* impl_;

public:
    /** Destructor.
    */
    ~serve_static();

    /** Construct with document root and default options.

        @param root The document root path.
    */
    explicit serve_static(core::string_view root);

    /** Construct with document root and options.

        @param root The document root path.

        @param opts Configuration options.
    */
    serve_static(
        core::string_view root,
        serve_static_options const& opts);

    /** Move constructor.
    */
    serve_static(serve_static&& other) noexcept;

    /** Handle a request.

        Attempts to serve the requested file from the document root.
        Sets appropriate headers and streams the file content.

        @param rp The route parameters.

        @return A task that completes with the routing result.
    */
    route_task operator()(route_params& rp) const;
};

} // http
} // boost

#endif
