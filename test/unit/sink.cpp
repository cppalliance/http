//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/buffers
//

// Test that header file is self-contained.
#include <boost/http/sink.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

struct sink_test
{
    struct test_sink : sink
    {
        std::string s_;
        std::size_t fail_;

        explicit
        test_sink(
            std::size_t fail) noexcept
            : fail_(fail)
        {
        }

        std::string const&
        str() const noexcept
        {
            return s_;
        }

        results
        on_write(
            capy::const_buffer b,
            bool) override
        {
            results rv;
            if(fail_-- == 0)
            {
                rv.ec = system::error_code(
                    system::errc::invalid_argument,
                    system::generic_category());
                return rv;
            }
            s_.append(static_cast<
                char const*>(b.data()),
                    b.size());
            rv.bytes += b.size();
            return rv;
        }
    };

    void
    testSink()
    {
        auto const& pat = test_pattern();

        // write(ConstBufferSequence)
        for(std::size_t i = 0;;++i)
        {
            test_sink dest(i);
            capy::const_buffer cb[3] = {
                { &pat[0], 3 },
                { &pat[3], 5 },
                { &pat[8], 7 } };
            boost::span<capy::const_buffer const> bs(cb);
            auto rv = dest.write(bs, false);
            if(rv.ec.failed())
                continue;
            BOOST_TEST_EQ(dest.str(), pat);
            break;
        }

        // write(const_buffer)
        for(std::size_t i = 0;;++i)
        {
            test_sink dest(i);
            capy::const_buffer cb(
                &pat[0], pat.size());
            auto rv = dest.write(cb, false);
            if(rv.ec.failed())
                continue;
            BOOST_TEST_EQ(dest.str(), pat);
            break;
        }

        // write(mutable_buffer)
        for(std::size_t i = 0;;++i)
        {
            test_sink dest(i);
            std::string s = pat;
            capy::mutable_buffer mb(
                &s[0], s.size());
            auto rv = dest.write(mb, false);
            if(rv.ec.failed())
                continue;
            BOOST_TEST_EQ(dest.str(), pat);
            break;
        }

        // empty sequence
        {
            test_sink dest(99);
            auto rv = dest.write(
                boost::span<capy::const_buffer const>{},
                true);
            BOOST_TEST(! rv.ec.failed());
            BOOST_TEST_EQ(rv.bytes, 0);
        }

        // sink::results aggregate workaround
        {
            sink::results rs{ {}, 0 };
            (void)rs;
        }
    }

    void
    run()
    {
        testSink();
    }
};

TEST_SUITE(
    sink_test,
    "boost.http.sink");

} // http
} // boost
