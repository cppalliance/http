//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// we need a pragma once for the circular includes required
// clangd's intellisense
#pragma once

#ifndef BOOST_HTTP_IMPL_PARSER_HPP
#define BOOST_HTTP_IMPL_PARSER_HPP

#include <boost/http/parser.hpp>
#include <boost/http/sink.hpp>
#include <boost/http/detail/type_traits.hpp>

namespace boost {
namespace http {

template<
    class Sink,
    class... Args,
    class>
Sink&
parser::
set_body(Args&&... args)
{
    // body must not already be set
    if(is_body_set())
        detail::throw_logic_error();

    // headers must be complete
    if(! got_header())
        detail::throw_logic_error();

    auto& s = ws().emplace<Sink>(
        std::forward<Args>(args)...);

    set_body_impl(s);
    return s;
}

} // http
} // boost

#endif
