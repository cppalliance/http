//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_RFC_LIST_RULE_HPP
#define BOOST_HTTP_RFC_LIST_RULE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/url/grammar/range_rule.hpp>
#include <boost/core/empty_value.hpp>

namespace boost {
namespace http {

namespace implementation_defined {
template<class Rule>
struct list_rule_t
    : private empty_value<Rule>
{
    using value_type = grammar::range<
        typename Rule::value_type>;

    constexpr
    list_rule_t(
        Rule const& r,
        std::size_t n,
        std::size_t m) noexcept
        : empty_value<Rule>(
            empty_init, r)
        , n_(n)
        , m_(m)
    {
    }

    auto
    parse(
        char const*& it,
        char const* end) const ->
            system::result<value_type>;

private:
    struct first_rule;
    struct next_rule;

    std::size_t n_;
    std::size_t m_;
};
} // implementation_defined

/** Rule for a comma-delimited list of elements

    This rule defines a list containing
    at least n and at most m of Element,
    each separated by at least one comma
    and optional whitespace.

    @par BNF
    @code
    #element       => [ 1#element ]
    1#element      => element *( OWS "," OWS element )
    <n>#<m>element => element <n-1>*<m-1>( OWS "," OWS element )
    @endcode

    Senders must emit compliant values, but
    receivers should accept values generated
    with the legacy production rules:

    @par Legacy BNF
    @code
    #element    => [ ( "," / element ) *( OWS "," [ OWS element ] ) ]

    1#element   => *( "," OWS ) element *( OWS "," [ OWS element ] )
    @endcode

    @tparam Rule The rule type.

    @param r The rule to use for elements.
    @param n The minimum number of elements, which may be zero.
    @param m The maximum number of elements.

    @return A rule that matches the list.

    @par Specification
    @li <a href="https://datatracker.ietf.org/doc/html/rfc7230#section-7"
        >5.6.1. Lists (#rule ABNF Extension) (rfc7230)</a>
*/
template<class Rule>
constexpr
auto
list_rule(
    Rule const& r,
    std::size_t n = 0,
    std::size_t m =
        std::size_t(-1)) noexcept ->
    implementation_defined::list_rule_t<Rule>
{
    return implementation_defined::list_rule_t<Rule>(r, n, m);
}

} // http
} // boost

#include <boost/http/rfc/impl/list_rule.hpp>

#endif
