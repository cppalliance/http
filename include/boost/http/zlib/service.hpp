//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZLIB_SERVICE_HPP
#define BOOST_HTTP_ZLIB_SERVICE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/ex/system_context.hpp>

namespace boost {
namespace http {
namespace zlib {

struct inflate_service;
struct deflate_service;

/** Install the inflate service.

    Installs the inflate service into the specified execution context.

    @param ctx The execution context to install into.

    @return A reference to the installed inflate service.
*/
BOOST_HTTP_DECL
inflate_service&
install_inflate_service(
    capy::execution_context& ctx);

/** Install the deflate service.

    Installs the deflate service into the specified execution context.

    @param ctx The execution context to install into.

    @return A reference to the installed deflate service.
*/
BOOST_HTTP_DECL
deflate_service&
install_deflate_service(
    capy::execution_context& ctx);

/** Install the ZLib inflate and deflate services, if available

    The services are installed into the system context,
    obtained by calling @ref capy::get_system_context.
*/
BOOST_HTTP_DECL
void
install_zlib_service();

} // zlib
} // http
} // boost

#endif
