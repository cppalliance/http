//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/server/cors.hpp>
#include "src/rfc/detail/rules.hpp"

#include "test_suite.hpp"

namespace boost {
namespace http_proto {

class field_item
{
public:
    field_item(
        core::string_view s)
        : s_(s)
    {
        grammar::parse(s_,
            detail::field_name_rule).value();
    }

    field_item(
        field f) noexcept
        : s_(to_string(f))
    {
    }

    operator core::string_view() const noexcept
    {
        return s_;
    }

private:
    core::string_view s_;
};

template<class Element>
struct list
{
    struct item
    {
        core::string_view s;

        template<
            class T,
            class = typename std::enable_if<
                std::is_constructible<
                    Element, T>::value>::type>
        item(T&& t)
            : s(Element(std::forward<T>(t)))
        {
        }
    };

public:
    list(std::initializer_list<item> init)
    {
        if(init.size() == 0)
            return;
        auto it = init.begin();
        s_ = it->s;
        while(++it != init.end())
        {
            s_.push_back(',');
            s_.append(it->s.data(),
                it->s.size());
        }
    }

    core::string_view get() const noexcept
    {
        return s_;
    }

private:
    std::string s_;
};

struct cors_test
{
    void run()
    {
        list<field_item> v({
            field::access_control_allow_origin,
            "example.com",
            "example.org"
            });
    }
};

TEST_SUITE(
    cors_test,
    "boost.http_proto.server.cors");

} // http_proto
} // boost
