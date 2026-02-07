//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/accepts.hpp>
#include <boost/http/server/mime_types.hpp>
#include <boost/http/field.hpp>
#include <algorithm>

namespace boost {
namespace http {

namespace {

//----------------------------------------------------------
// Helpers
//----------------------------------------------------------

std::string_view
trim_ows( std::string_view s ) noexcept
{
    while( ! s.empty() &&
        ( s.front() == ' ' || s.front() == '\t' ) )
        s.remove_prefix( 1 );
    while( ! s.empty() &&
        ( s.back() == ' ' || s.back() == '\t' ) )
        s.remove_suffix( 1 );
    return s;
}

bool
iequals(
    std::string_view a,
    std::string_view b ) noexcept
{
    if( a.size() != b.size() )
        return false;
    for( std::size_t i = 0; i < a.size(); ++i )
    {
        unsigned char ca = a[i];
        unsigned char cb = b[i];
        if( ca >= 'A' && ca <= 'Z' )
            ca += 32;
        if( cb >= 'A' && cb <= 'Z' )
            cb += 32;
        if( ca != cb )
            return false;
    }
    return true;
}

// Returns quality as integer 0-1000
int
parse_q( std::string_view s ) noexcept
{
    s = trim_ows( s );
    if( s.empty() )
        return 1000;
    if( s[0] == '1' )
        return 1000;
    if( s[0] != '0' )
        return 0;
    if( s.size() < 2 || s[1] != '.' )
        return 0;
    int result = 0;
    int mult = 100;
    for( std::size_t i = 2;
        i < s.size() && i < 5; ++i )
    {
        if( s[i] < '0' || s[i] > '9' )
            break;
        result += ( s[i] - '0' ) * mult;
        mult /= 10;
    }
    return result;
}

// Extract q-value from parameters after first semicolon
int
extract_q( std::string_view params ) noexcept
{
    while( ! params.empty() )
    {
        auto semi = params.find( ';' );
        auto param = trim_ows(
            semi != std::string_view::npos
                ? params.substr( 0, semi )
                : params );
        if( param.size() >= 2 &&
            ( param[0] == 'q' || param[0] == 'Q' ) &&
            param[1] == '=' )
        {
            return parse_q( param.substr( 2 ) );
        }
        if( semi != std::string_view::npos )
            params.remove_prefix( semi + 1 );
        else
            break;
    }
    return 1000;
}

//----------------------------------------------------------
// Negotiation priority
//----------------------------------------------------------

struct priority
{
    int q;
    int specificity;
    int order;
};

bool
is_better(
    priority const& a,
    priority const& b ) noexcept
{
    if( a.q != b.q )
        return a.q > b.q;
    if( a.specificity != b.specificity )
        return a.specificity > b.specificity;
    return a.order < b.order;
}

//----------------------------------------------------------
// Media type parsing (Accept header)
//----------------------------------------------------------

struct media_range
{
    std::string_view type;
    std::string_view subtype;
    std::string_view full;
    int q;
    int order;
};

std::vector<media_range>
parse_accept( std::string_view header )
{
    std::vector<media_range> result;
    int order = 0;

    while( ! header.empty() )
    {
        auto comma = header.find( ',' );
        auto entry = ( comma != std::string_view::npos )
            ? header.substr( 0, comma )
            : header;
        if( comma != std::string_view::npos )
            header.remove_prefix( comma + 1 );
        else
            header = {};

        entry = trim_ows( entry );
        if( entry.empty() )
            continue;

        auto semi = entry.find( ';' );
        auto mime_part = trim_ows(
            semi != std::string_view::npos
                ? entry.substr( 0, semi )
                : entry );

        auto slash = mime_part.find( '/' );
        if( slash == std::string_view::npos )
            continue;

        media_range mr;
        mr.type = mime_part.substr( 0, slash );
        mr.subtype = mime_part.substr( slash + 1 );
        mr.full = mime_part;
        mr.q = ( semi != std::string_view::npos )
            ? extract_q( entry.substr( semi + 1 ) )
            : 1000;
        mr.order = order++;
        result.push_back( mr );
    }

    return result;
}

// Returns specificity (0-6) or -1 for no match
int
match_media(
    media_range const& range,
    std::string_view type,
    std::string_view subtype ) noexcept
{
    int s = 0;

    if( range.type == "*" )
    {
        // wildcard type
    }
    else if( iequals( range.type, type ) )
    {
        s |= 4;
    }
    else
    {
        return -1;
    }

    if( range.subtype == "*" )
    {
        // wildcard subtype
    }
    else if( iequals( range.subtype, subtype ) )
    {
        s |= 2;
    }
    else
    {
        return -1;
    }

    return s;
}

//----------------------------------------------------------
// Simple token parsing (Accept-Encoding/Charset/Language)
//----------------------------------------------------------

struct simple_entry
{
    std::string_view value;
    int q;
    int order;
};

std::vector<simple_entry>
parse_simple( std::string_view header )
{
    std::vector<simple_entry> result;
    int order = 0;

    while( ! header.empty() )
    {
        auto comma = header.find( ',' );
        auto entry = ( comma != std::string_view::npos )
            ? header.substr( 0, comma )
            : header;
        if( comma != std::string_view::npos )
            header.remove_prefix( comma + 1 );
        else
            header = {};

        entry = trim_ows( entry );
        if( entry.empty() )
            continue;

        auto semi = entry.find( ';' );
        auto value = trim_ows(
            semi != std::string_view::npos
                ? entry.substr( 0, semi )
                : entry );
        if( value.empty() )
            continue;

        simple_entry se;
        se.value = value;
        se.q = ( semi != std::string_view::npos )
            ? extract_q( entry.substr( semi + 1 ) )
            : 1000;
        se.order = order++;
        result.push_back( se );
    }

    return result;
}

//----------------------------------------------------------
// Matching helpers
//----------------------------------------------------------

// Exact or wildcard match (encoding, charset)
int
match_exact(
    std::string_view spec,
    std::string_view offered ) noexcept
{
    if( iequals( spec, offered ) )
        return 1;
    if( spec == "*" )
        return 0;
    return -1;
}

// Language prefix: "en-US" -> "en"
std::string_view
lang_prefix( std::string_view tag ) noexcept
{
    auto dash = tag.find( '-' );
    if( dash != std::string_view::npos )
        return tag.substr( 0, dash );
    return tag;
}

// Language match with prefix support
int
match_language(
    std::string_view spec,
    std::string_view offered ) noexcept
{
    if( iequals( spec, offered ) )
        return 4;
    if( iequals( lang_prefix( spec ), offered ) )
        return 2;
    if( iequals( spec, lang_prefix( offered ) ) )
        return 1;
    if( spec == "*" )
        return 0;
    return -1;
}

//----------------------------------------------------------
// Generic negotiation for simple headers
//----------------------------------------------------------

template< class MatchFn >
std::string_view
negotiate(
    std::vector<simple_entry> const& entries,
    std::initializer_list<std::string_view> offered,
    MatchFn match )
{
    std::string_view best_val;
    priority best_pri{ -1, -1, 0 };
    bool found = false;

    for( auto const& o : offered )
    {
        priority pri{ -1, -1, 0 };
        bool matched = false;

        for( auto const& e : entries )
        {
            if( e.q <= 0 )
                continue;
            auto s = match( e.value, o );
            if( s < 0 )
                continue;
            priority p{ e.q, s, e.order };
            if( ! matched ||
                p.specificity > pri.specificity ||
                ( p.specificity == pri.specificity &&
                    p.q > pri.q ) ||
                ( p.specificity == pri.specificity &&
                    p.q == pri.q &&
                    p.order < pri.order ) )
            {
                pri = p;
                matched = true;
            }
        }

        if( ! matched || pri.q <= 0 )
            continue;

        if( ! found || is_better( pri, best_pri ) )
        {
            best_val = o;
            best_pri = pri;
            found = true;
        }
    }

    return found ? best_val : std::string_view{};
}

// Return sorted values from simple entries
std::vector<std::string_view>
sorted_values(
    std::vector<simple_entry>& entries )
{
    std::sort( entries.begin(), entries.end(),
        []( simple_entry const& a,
            simple_entry const& b )
        {
            if( a.q != b.q )
                return a.q > b.q;
            return a.order < b.order;
        });

    std::vector<std::string_view> result;
    result.reserve( entries.size() );
    for( auto const& e : entries )
    {
        if( e.q <= 0 )
            continue;
        result.push_back( e.value );
    }
    return result;
}

} // (anon)

//----------------------------------------------------------

accepts::accepts(
    fields_base const& fields ) noexcept
    : fields_( fields )
{
}

std::string_view
accepts::type(
    std::initializer_list<
        std::string_view> offered ) const
{
    if( offered.size() == 0 )
        return {};

    auto accept = fields_.value_or(
        field::accept, "" );

    if( accept.empty() )
        return *offered.begin();

    auto ranges = parse_accept( accept );
    if( ranges.empty() )
        return *offered.begin();

    std::string_view best_val;
    priority best_pri{ -1, -1, 0 };
    bool found = false;

    for( auto const& o : offered )
    {
        // Convert extension to MIME if needed
        std::string_view mime_str = o;
        if( o.find( '/' ) == std::string_view::npos )
        {
            auto looked = mime_types::lookup( o );
            if( ! looked.empty() )
                mime_str = looked;
            else
                continue;
        }

        auto slash = mime_str.find( '/' );
        if( slash == std::string_view::npos )
            continue;

        auto type = mime_str.substr( 0, slash );
        auto subtype = mime_str.substr( slash + 1 );

        // Find best matching range for this type
        priority pri{ -1, -1, 0 };
        bool matched = false;

        for( auto const& r : ranges )
        {
            if( r.q <= 0 )
                continue;
            auto s = match_media( r, type, subtype );
            if( s < 0 )
                continue;
            priority p{ r.q, s, r.order };
            if( ! matched ||
                p.specificity > pri.specificity ||
                ( p.specificity == pri.specificity &&
                    p.q > pri.q ) ||
                ( p.specificity == pri.specificity &&
                    p.q == pri.q &&
                    p.order < pri.order ) )
            {
                pri = p;
                matched = true;
            }
        }

        if( ! matched || pri.q <= 0 )
            continue;

        if( ! found || is_better( pri, best_pri ) )
        {
            best_val = o;
            best_pri = pri;
            found = true;
        }
    }

    return found ? best_val : std::string_view{};
}

std::vector<std::string_view>
accepts::types() const
{
    auto accept = fields_.value_or(
        field::accept, "" );
    if( accept.empty() )
        return {};

    auto ranges = parse_accept( accept );

    std::sort( ranges.begin(), ranges.end(),
        []( media_range const& a,
            media_range const& b )
        {
            if( a.q != b.q )
                return a.q > b.q;
            return a.order < b.order;
        });

    std::vector<std::string_view> result;
    result.reserve( ranges.size() );
    for( auto const& r : ranges )
    {
        if( r.q <= 0 )
            continue;
        result.push_back( r.full );
    }
    return result;
}

std::string_view
accepts::encoding(
    std::initializer_list<
        std::string_view> offered ) const
{
    if( offered.size() == 0 )
        return {};

    auto header = fields_.value_or(
        field::accept_encoding, "" );

    if( header.empty() )
        return *offered.begin();

    auto entries = parse_simple( header );
    if( entries.empty() )
        return *offered.begin();

    return negotiate( entries, offered, match_exact );
}

std::vector<std::string_view>
accepts::encodings() const
{
    auto header = fields_.value_or(
        field::accept_encoding, "" );
    if( header.empty() )
        return {};

    auto entries = parse_simple( header );
    return sorted_values( entries );
}

std::string_view
accepts::charset(
    std::initializer_list<
        std::string_view> offered ) const
{
    if( offered.size() == 0 )
        return {};

    auto header = fields_.value_or(
        field::accept_charset, "" );

    if( header.empty() )
        return *offered.begin();

    auto entries = parse_simple( header );
    if( entries.empty() )
        return *offered.begin();

    return negotiate( entries, offered, match_exact );
}

std::vector<std::string_view>
accepts::charsets() const
{
    auto header = fields_.value_or(
        field::accept_charset, "" );
    if( header.empty() )
        return {};

    auto entries = parse_simple( header );
    return sorted_values( entries );
}

std::string_view
accepts::language(
    std::initializer_list<
        std::string_view> offered ) const
{
    if( offered.size() == 0 )
        return {};

    auto header = fields_.value_or(
        field::accept_language, "" );

    if( header.empty() )
        return *offered.begin();

    auto entries = parse_simple( header );
    if( entries.empty() )
        return *offered.begin();

    return negotiate( entries, offered, match_language );
}

std::vector<std::string_view>
accepts::languages() const
{
    auto header = fields_.value_or(
        field::accept_language, "" );
    if( header.empty() )
        return {};

    auto entries = parse_simple( header );
    return sorted_values( entries );
}

} // http
} // boost
