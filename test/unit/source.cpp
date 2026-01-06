//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/buffers
//

// Test that header file is self-contained.
#include <boost/http/source.hpp>

#include <boost/buffers/slice.hpp>
#include <boost/buffers/range.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

struct source_test
{
    struct test_source : source
    {
        buffers::const_buffer cb_;
        std::size_t fail_;

        explicit
        test_source(
            std::size_t fail) noexcept
            : fail_(fail)
        {
            auto const& pat = test_pattern();
            cb_ = { &pat[0], pat.size() };
        }

        results
        on_read(
            buffers::mutable_buffer b) override
        {
            results rv;
            if(fail_-- == 0)
            {
                rv.ec = boost::system::error_code(
                    boost::system::errc::invalid_argument,
                    boost::system::generic_category());
                return rv;
            }
            auto n = buffers::copy(b, cb_);
            buffers::remove_prefix(cb_, n);
            rv.bytes += n;
            rv.finished = cb_.size() == 0;
            return rv;
        }
    };

    void
    testSource()
    {
        auto const& pat = test_pattern();

        // read(MutableBufferSequence)
        for(std::size_t i = 0;;++i)
        {
            test_source src(i);
            std::string s(
                pat.size(), 0);
            buffers::mutable_buffer mb[3] = {
                { &s[0], 3 },
                { &s[3], 5 },
                { &s[8], 7 } };
            boost::span<buffers::mutable_buffer const> bs(mb);
            auto rv = src.read(bs);
            if(rv.ec.failed())
                continue;
            BOOST_TEST(rv.finished);
            BOOST_TEST_EQ(
                rv.bytes, pat.size());
            s.resize(rv.bytes);
            BOOST_TEST_EQ(s, pat);
            break;
        }

        // read(mutable_buffer)
        for(std::size_t i = 0;;++i)
        {
            test_source src(i);
            std::string s(
                pat.size(), 0);
            buffers::mutable_buffer mb(
                &s[0], s.size());
            auto rv = src.read(mb);
            if(rv.ec.failed())
                continue;
            BOOST_TEST(rv.finished);
            BOOST_TEST_EQ(
                rv.bytes, pat.size());
            s.resize(rv.bytes);
            BOOST_TEST_EQ(s, pat);
            break;
        }

        // empty sequence
        {
            test_source src(99);
            auto rv = src.read(
                boost::span<buffers::mutable_buffer const>{});
            BOOST_TEST(! rv.ec.failed());
            BOOST_TEST_EQ(rv.bytes, 0);
        }

        // source::results aggregate workaround
        {
            source::results rs{ {}, 0, false };
            (void)rs;
        }
    }

    void
    run()
    {
        testSource();
    }
};

TEST_SUITE(
    source_test,
    "boost.http.source");

} // http
} // boost
