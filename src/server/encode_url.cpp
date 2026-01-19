//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/encode_url.hpp>

namespace boost {
namespace http {

namespace {

// Check if character needs encoding
// Unreserved + reserved chars that are allowed in URLs
bool
is_safe( char c ) noexcept
{
    // Unreserved: A-Z a-z 0-9 - _ . ~
    if( ( c >= 'A' && c <= 'Z' ) ||
        ( c >= 'a' && c <= 'z' ) ||
        ( c >= '0' && c <= '9' ) ||
        c == '-' || c == '_' || c == '.' || c == '~' )
        return true;

    // Reserved chars allowed in URLs: ! # $ & ' ( ) * + , / : ; = ? @
    switch( c )
    {
    case '!':
    case '#':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '/':
    case ':':
    case ';':
    case '=':
    case '?':
    case '@':
        return true;
    default:
        return false;
    }
}

constexpr char hex_chars[] = "0123456789ABCDEF";

} // (anon)

std::string
encode_url( core::string_view url )
{
    std::string result;
    result.reserve( url.size() );

    for( unsigned char c : url )
    {
        if( is_safe( static_cast<char>( c ) ) )
        {
            result.push_back( static_cast<char>( c ) );
        }
        else
        {
            result.push_back( '%' );
            result.push_back( hex_chars[c >> 4] );
            result.push_back( hex_chars[c & 0x0F] );
        }
    }

    return result;
}

} // http
} // boost
