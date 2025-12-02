//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

// Test that header file is self-contained.
#include <boost/http_proto/error.hpp>
#include <boost/url/grammar/error.hpp>

#include <memory>

#include "test_suite.hpp"

namespace boost {
namespace http_proto {

class error_test
{
public:
    void
    check(
        char const* name,
        error ev)
    {
        auto const ec =
          make_error_code(ev);
        auto const esuccess =
          make_error_code(static_cast<error>(0));
        auto const& cat =
         esuccess.category();
        BOOST_TEST(std::string(ec.category().name()) == name);
        BOOST_TEST(! ec.message().empty());
        BOOST_TEST(
            std::addressof(ec.category()) == std::addressof(cat));
        BOOST_TEST(cat.equivalent(
            static_cast<std::underlying_type<error>::type>(ev),
                ec.category().default_error_condition(
                    static_cast<std::underlying_type<error>::type>(ev))));
        BOOST_TEST(cat.equivalent(ec,
            static_cast<std::underlying_type<error>::type>(ev)));
    }

    void
    check(
        std::string name,
        condition c,
        system::error_code ec)
    {
        {
            BOOST_TEST_NE(
                ec.category().name(), nullptr);
            BOOST_TEST(! ec.message().empty());
            BOOST_TEST_EQ(ec, c);
        }
        {
            auto cc = make_error_condition(c);
            BOOST_TEST_EQ(std::string(
                cc.category().name()), name);
            BOOST_TEST(! cc.message().empty());
            BOOST_TEST_EQ(cc, c);
        }
    }

    void
    run()
    {
        char const* const n = "boost.http";

        check(n, error::expect_100_continue);
        check(n, error::end_of_message);
        check(n, error::end_of_stream);
        check(n, error::in_place_overflow);
        check(n, error::need_data);

        check(n, error::bad_connection);
        check(n, error::bad_content_length);
        check(n, error::bad_expect);
        check(n, error::bad_field_name);
        check(n, error::bad_field_value);
        check(n, error::bad_field_smuggle);
        check(n, error::bad_line_ending);
        check(n, error::bad_list);
        check(n, error::bad_method);
        check(n, error::bad_number);
        check(n, error::bad_payload);
        check(n, error::bad_version);
        check(n, error::bad_reason);
        check(n, error::bad_request_target);
        check(n, error::bad_status_code);
        check(n, error::bad_status_line);
        check(n, error::bad_transfer_encoding);
        check(n, error::bad_upgrade);

        check(n, error::body_too_large);
        check(n, error::headers_limit);
        check(n, error::start_line_limit);
        check(n, error::field_size_limit);
        check(n, error::fields_limit);
        check(n, error::incomplete);

        check(n, error::numeric_overflow);
        check(n, error::multiple_content_length);

        check(n, error::buffer_overflow);

        //---

        check(n,
            condition::need_more_input,
            urls::grammar::error::need_more);
        check(n,
             condition::invalid_payload,
             error::bad_payload);
    }
};

TEST_SUITE(error_test, "boost.http_proto.error");

} // http_proto
} // boost
