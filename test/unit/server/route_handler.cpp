//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/server/route_handler.hpp>

#include <boost/http/server/router.hpp>
#include <boost/http/server/flat_router.hpp>
#include <boost/http/request.hpp>
#include <boost/capy/ex/run_sync.hpp>

#include "test_route_handler.hpp"
#include "test_suite.hpp"

namespace boost {
namespace http {

struct route_handler_test
{
    using test_router = router<route_params>;

    void check(
        test_router& r,
        http::method verb,
        core::string_view url,
        route_result rv0 = route_result{})
    {
        flat_router fr(std::move(r));
        test_route_params p;
        auto rv = capy::run_sync()(fr.dispatch(
            verb, urls::url_view(url), p));
        if(BOOST_TEST_EQ(rv.message(), rv0.message()))
            BOOST_TEST(rv == rv0);
    }

    void testData()
    {
        static auto const POST = http::method::post;

        struct auth_token
        {
            bool valid = false;
        };

        struct session_token
        {
            bool valid = false;
        };

        test_router r;
        r.use(
            [](route_params& p) -> capy::task<route_result>
            {
                // create session_token
                auto& st = p.session_data.try_emplace<session_token>();
                BOOST_TEST_EQ(st.valid, false);
                co_return route::next;
            });
        r.use("/user",
            [](route_params& p) -> capy::task<route_result>
            {
                // make session token valid
                auto* st = p.session_data.find<session_token>();
                if(BOOST_TEST_NE(st, nullptr))
                    st->valid = true;
                co_return route::next;
            });
        r.route("/user/auth")
            .add(POST,
                [](route_params& p) -> capy::task<route_result>
                {
                    auto& st = p.session_data.get<session_token>();
                    BOOST_TEST_EQ(st.valid, true);
                    // create auth_token each time
                    auto& at = p.route_data.emplace<auth_token>();
                    at.valid = true;
                    co_return route::next;
                },
                [](route_params& p) -> capy::task<route_result>
                {
                    auto& at = p.route_data.get<auth_token>();
                    auto& st = p.session_data.get<session_token>();
                    BOOST_TEST_EQ(at.valid, true);
                    BOOST_TEST_EQ(st.valid, true);
                    co_return route_result{};
                });
        check(r, POST, urls::url_view("/user/auth"));
    }

    void run()
    {
        testData();
    }
};

TEST_SUITE(
    route_handler_test,
    "boost.http.server.route_handler");

} // http
} // boost
