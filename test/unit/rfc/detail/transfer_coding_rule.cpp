//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include "src/rfc/detail/transfer_coding_rule.hpp"

#include "test_helpers.hpp"

#include <boost/http/rfc/list_rule.hpp>
#include <boost/static_assert.hpp>
#include <type_traits>

#if defined(BOOST_HTTP_STATIC_LINK)

namespace boost {
namespace http {
namespace detail {

BOOST_STATIC_ASSERT(
    std::is_nothrow_copy_assignable<
        system::result<transfer_coding_rule_t::value_type>>::value);

BOOST_STATIC_ASSERT(
    std::is_nothrow_copy_constructible<
        system::result<transfer_coding_rule_t::value_type>>::value);

struct transfer_coding_rule_test
{
    void
    run()
    {
        auto const& t =
            list_rule(transfer_coding_rule, 1);

        bad(t, "");
        bad(t, " ");
        bad(t, " x");
        bad(t, "x ");
        bad(t, " x ");
        ok(t,  "chunked");
        ok(t,  "compress");
        ok(t,  "deflate");
        ok(t,  "gzip");
        ok(t,  "wakanda;status=good");
        ok(t,  "wakanda;x=1;y=2");
        ok(t,  "main;dir=\"Program\\ Files\"");
        ok(t,  "main;dir1=\"Program\\ Files\";dir2=master");
        ok(t,  "gzip,chunked");
        ok(t,  "gzip, chunked");
        ok(t,  "br;level=5,chunked");
        ok(t,  "br;level=5, chunked");
        ok(t,  "br;level=5, ,chunked");
        ok(t,  "br;level=5, ,chunked,");
        ok(t,  "br;level=5, ,chunked, ,");
        bad(t, "br;level=5, ,chunked, ");

        ok(t, "chunked");
        ok(t, "compress");
        ok(t, "deflate");
        ok(t, "gzip");

        ok(t, "Chunked");
        ok(t, "Compress");
        ok(t, "Deflate");
        ok(t, "Gzip");

        bad(t, "chunked;p=2");
        bad(t, "compress;p=2");
        bad(t, "deflate;p=2");
        bad(t, "gzip;p=2");
    }
};

TEST_SUITE(
    transfer_coding_rule_test,
    "boost.http.detail.transfer_coding_rule");

} // detail
} // http
} // boost

#endif // defined(BOOST_HTTP_STATIC_LINK)
