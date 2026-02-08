//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_CONFIG_HPP
#define BOOST_HTTP_DETAIL_CONFIG_HPP

#include <boost/config.hpp>
#include <stdint.h>

namespace boost {

namespace http {

//------------------------------------------------

# if (defined(BOOST_HTTP_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)) && !defined(BOOST_HTTP_STATIC_LINK)
#  if defined(BOOST_HTTP_SOURCE)
#   define BOOST_HTTP_DECL        BOOST_SYMBOL_EXPORT
#   define BOOST_HTTP_BUILD_DLL
#  else
#   define BOOST_HTTP_DECL        BOOST_SYMBOL_IMPORT
#  endif
# endif // shared lib

# ifndef  BOOST_HTTP_DECL
#  define BOOST_HTTP_DECL
# endif

#if defined(__MINGW32__)
    #define BOOST_HTTP_SYMBOL_VISIBLE BOOST_HTTP_DECL
#else
    #define BOOST_HTTP_SYMBOL_VISIBLE BOOST_SYMBOL_VISIBLE
#endif

# if !defined(BOOST_HTTP_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_HTTP_NO_LIB)
#  define BOOST_LIB_NAME boost_http
#  if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_HTTP_DYN_LINK)
#   define BOOST_DYN_LINK
#  endif
#  include <boost/config/auto_link.hpp>
# endif

//-----------------------------------------------

#if defined(__cpp_lib_coroutine) && __cpp_lib_coroutine >= 201902L
# define BOOST_HTTP_HAS_CORO 1
#elif defined(__cpp_impl_coroutine) && __cpp_impl_coroutines >= 201902L
# define BOOST_HTTP_HAS_CORO 1
#endif

//-----------------------------------------------

#if defined(BOOST_NO_CXX14_AGGREGATE_NSDMI)
# define BOOST_HTTP_AGGREGATE_WORKAROUND
#endif

// Add source location to error codes
#ifdef BOOST_HTTP_NO_SOURCE_LOCATION
# define BOOST_HTTP_ERR(ev) (::boost::system::error_code(ev))
# define BOOST_HTTP_RETURN_EC(ev) return (ev)
#else
# define BOOST_HTTP_ERR(ev) ( \
    ::boost::system::error_code( (ev), [] { \
    static constexpr auto loc((BOOST_CURRENT_LOCATION)); \
    return &loc; }()))
# define BOOST_HTTP_RETURN_EC(ev)                                  \
    do {                                                                 \
        static constexpr auto loc ## __LINE__((BOOST_CURRENT_LOCATION)); \
        return ::boost::system::error_code((ev), &loc ## __LINE__);      \
    } while(0)
#endif

} // http

// lift grammar into our namespace
namespace urls {
namespace grammar {}
}
namespace http {
namespace grammar = ::boost::urls::grammar;
} // http

} // boost

#endif
