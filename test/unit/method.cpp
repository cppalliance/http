//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/method.hpp>

#include "test_suite.hpp"

namespace boost {
namespace http {

class method_test
{
public:
    void
    testVerb()
    {
        auto const good =
            [&](method v)
            {
                BOOST_TEST(string_to_method(
                    to_string(v)) == v);
            };

        good(method::unknown);

        good(method::delete_);
        good(method::get);
        good(method::head);
        good(method::post);
        good(method::put);
        good(method::connect);
        good(method::options);
        good(method::trace);
        good(method::copy);
        good(method::lock);
        good(method::mkcol);
        good(method::move);
        good(method::propfind);
        good(method::proppatch);
        good(method::search);
        good(method::unlock);
        good(method::bind);
        good(method::rebind);
        good(method::unbind);
        good(method::acl);
        good(method::report);
        good(method::mkactivity);
        good(method::checkout);
        good(method::merge);
        good(method::msearch);
        good(method::notify);
        good(method::subscribe);
        good(method::unsubscribe);
        good(method::patch);
        good(method::purge);
        good(method::mkcalendar);
        good(method::link);
        good(method::unlink);

        auto const bad =
            [&](core::string_view s)
            {
                auto const v = string_to_method(s);
                BOOST_TEST(v == method::unknown);
            };

        bad("AC_");
        bad("BIN_");
        bad("CHECKOU_");
        bad("CONNEC_");
        bad("COP_");
        bad("DELET_");
        bad("GE_");
        bad("HEA_");
        bad("LIN_");
        bad("LOC_");
        bad("M-SEARC_");
        bad("MERG_");
        bad("MKACTIVIT_");
        bad("MKCALENDA_");
        bad("MKCO_");
        bad("MOV_");
        bad("NOTIF_");
        bad("OPTION_");
        bad("PATC_");
        bad("POS_");
        bad("PROPFIN_");
        bad("PROPPATC_");
        bad("PURG_");
        bad("PU_");
        bad("REBIN_");
        bad("REPOR_");
        bad("SEARC_");
        bad("SUBSCRIB_");
        bad("TRAC_");
        bad("UNBIN_");
        bad("UNLIN_");
        bad("UNLOC_");
        bad("UNSUBSCRIB_");

        try
        {
            to_string(static_cast<method>(-1));
            BOOST_TEST_FAIL();
        }
        catch(std::exception const&)
        {
            BOOST_TEST_PASS();
        }
    }

    void
    run()
    {
        testVerb();
    }
};

TEST_SUITE(method_test, "boost.http.method");

} // http
} // boost
