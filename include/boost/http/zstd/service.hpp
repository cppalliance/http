//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_ZSTD_SERVICE_HPP
#define BOOST_HTTP_ZSTD_SERVICE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/capy/ex/system_context.hpp>

namespace boost {
namespace http {
namespace zstd {

struct compress_service;
struct decompress_service;

/** Install the compress service.

    Installs the compress service into the specified execution context.

    @param ctx The execution context to install into.

    @return A reference to the installed compress service.
*/
BOOST_HTTP_DECL
compress_service&
install_compress_service(
    capy::execution_context& ctx);

/** Install the decompress service.

    Installs the decompress service into the specified execution context.

    @param ctx The execution context to install into.

    @return A reference to the installed decompress service.
*/
BOOST_HTTP_DECL
decompress_service&
install_decompress_service(
    capy::execution_context& ctx);

/** Install the Zstandard compress and decompress services, if available.

    The services are installed into the system context,
    obtained by calling @ref capy::get_system_context.
*/
BOOST_HTTP_DECL
void
install_zstd_service();

} // zstd
} // http
} // boost

#endif
