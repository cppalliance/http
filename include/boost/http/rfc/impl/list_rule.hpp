//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_IMPL_LIST_RULE_HPP
#define BOOST_HTTP_RFC_IMPL_LIST_RULE_HPP

#include <boost/http/rfc/detail/ws.hpp>

#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/charset.hpp>
#include <boost/url/grammar/range_rule.hpp>
#include <boost/core/empty_value.hpp>

namespace boost {
namespace http {

namespace detail {

/*  Peter:

    So, to put everything together, this is what I propose

    - make range2_rule that takes first and next with
      value types of optional<E> rather than E like the current rule
    - make variant_rule produce an optional<X> when otherwise
      the value type would have been variant<void, void, X, void>
    - add operators for combining the rules so that one can
      write *( OWS >> !literal(",") >> -( OWS >> element ) )
    - profit
*/

// *( OWS "," )
struct ows_comma_t
{
    using value_type = void;

    auto
    parse(
        char const*& it,
        char const* end) const noexcept ->
            system::result<value_type>
    {
        auto it1 = it;
        while(it != end)
        {
            // SP / HT
            if( *it == ' ' ||
                *it == '\t')
            {
                ++it;
                continue;
            }
            if(*it != ',')
                break;
            // ","
            it1 = ++it;
        }
        it = it1;
        return {};
    }
};

constexpr ows_comma_t ows_comma{};

} // detail

/*
    #element    => [ ( "," / element ) *( OWS "," [ OWS element ] ) ]

    #element    => first *next
    first       => [ element / ( "," *( OWS "," ) [ OWS element ] ) ]
    next        => "" / ( 1*( OWS "," ) [ OWS element ] )
*/

namespace implementation_defined {
template<class Rule>
struct list_rule_t<Rule>::
    first_rule : empty_value<Rule>
{
    using value_type =
        typename Rule::value_type;

    constexpr
    explicit
    first_rule(
        Rule const& r) noexcept
        : empty_value<Rule>(
            empty_init, r)
    {
    }

    auto
    parse(
        char const*& it,
        char const* end) const ->
            system::result<value_type>
    {
    //  first       => [ element / ( "," *( OWS "," ) [ OWS element ] ) ]

        if(it == end)
            return grammar::error::end_of_range;
        {
            // element
            auto it0 = it;
            auto rv = this->get().parse(it, end);
            if(rv)
                return std::move(*rv);
            it = it0;
        }
        // ","
        if(*it != ',')
            return grammar::error::end_of_range;
        ++it;
        // *( OWS "," )
        detail::ows_comma.parse(it, end);
        auto it1 = it;
        // OWS
        it = grammar::find_if_not(
            it, end, detail::ws);
        // element
        auto rv = this->get().parse(it, end);
        if(rv)
            return std::move(*rv);
        it = it1;
        return grammar::error::end_of_range;
    }
};

template<class Rule>
struct list_rule_t<Rule>::
    next_rule : empty_value<Rule>
{
    using value_type =
        typename Rule::value_type;

    constexpr
    explicit
    next_rule(
        Rule const& r) noexcept
        : empty_value<Rule>(
            empty_init, r)
    {
    }

    auto
    parse(
        char const*& it,
        char const* end) const ->
            system::result<value_type>
    {
    //  next        => "" / ( 1*( OWS "," ) [ OWS element ] )

        // ""
        if(it == end)
            return grammar::error::end_of_range;

        // 1*( OWS "," )
        {
            auto it0 = it;
            detail::ows_comma.parse(it, end);
            if(it == it0)
                return grammar::error::end_of_range;
        }
        auto it1 = it;
        // OWS
        it = grammar::find_if_not(
            it, end, detail::ws);
        auto rv = this->get().parse(it, end);
        if(rv)
            return std::move(*rv);
        it = it1;
        return grammar::error::end_of_range;
    }
};

template<class Rule>
auto
list_rule_t<Rule>::
parse(
    char const*& it,
    char const* end) const ->
        system::result<value_type>
{
    return grammar::parse(it, end,
        grammar::range_rule(
            first_rule{this->get()},
            next_rule{this->get()},
                n_, m_));
}
} // implementation_defined

} // http
} // boost

#endif
