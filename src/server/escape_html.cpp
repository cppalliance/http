//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/escape_html.hpp>

namespace boost {
namespace http {

std::string
escape_html( core::string_view s )
{
    std::string result;
    result.reserve( s.size() );

    for( char c : s )
    {
        switch( c )
        {
        case '&':
            result.append( "&amp;" );
            break;
        case '<':
            result.append( "&lt;" );
            break;
        case '>':
            result.append( "&gt;" );
            break;
        case '"':
            result.append( "&quot;" );
            break;
        case '\'':
            result.append( "&#39;" );
            break;
        default:
            result.push_back( c );
            break;
        }
    }

    return result;
}

} // http
} // boost
