//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_BROTLI_SERVICE_HPP
#define BOOST_HTTP_BROTLI_SERVICE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/ex/system_context.hpp>

namespace boost {
namespace http {
namespace brotli {

struct decode_service;
struct encode_service;

/** Install the decode service.

    Installs the decode service into the specified execution context.

    @param ctx The execution context to install into.

    @return A reference to the installed decode service.
*/
BOOST_HTTP_DECL
decode_service&
install_decode_service(
    capy::execution_context& ctx);

/** Install the encode service.

    Installs the encode service into the specified execution context.

    @param ctx The execution context to install into.

    @return A reference to the installed encode service.
*/
BOOST_HTTP_DECL
encode_service&
install_encode_service(
    capy::execution_context& ctx);

/** Install the Brotli encode and decode services, if available.

    The services are installed into the system context,
    obtained by calling @ref capy::get_system_context.
*/
BOOST_HTTP_DECL
void
install_brotli_service();

} // brotli
} // http
} // boost

#endif
