//
// Copyright (c) 2022 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/file_source.hpp>

#include <boost/capy/buffers/make_buffer.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

#include "test_suite.hpp"

namespace boost {
namespace http {

struct file_source_test
{
    class temp_path
    {
        std::string path_;

    public:
        temp_path()
            // filesystem::path::string() fails on older
            // versions of mingw when rtti is off
            : path_(filesystem::unique_path().string())
        {
        }

        operator char const*() const noexcept
        {
            return path_.c_str();
        }

        ~temp_path()
        {
            filesystem::remove(path_);
        }
    };

    void
    write_file(
        temp_path const& path,
        core::string_view content)
    {
        std::ofstream ofs(path);
        ofs << content;
    }

    void
    testReportErros()
    {
        // passing a closed file
        {
            capy::file f;
            file_source fsource(std::move(f));
            char buf[16] = {};
            auto rs = fsource.read(
                capy::make_buffer(buf));
            BOOST_TEST_EQ(rs.bytes, 0);
            BOOST_TEST(rs.ec.failed());
            BOOST_TEST(!rs.finished);
        }
    }

    void
    testRead()
    {
        temp_path path;
        write_file(path, "Hello, World!");
        capy::file f;
        system::error_code ec;
        f.open(path, capy::file_mode::read, ec);
        BOOST_TEST(!ec);
        BOOST_TEST(f.is_open());
        file_source fsource(std::move(f));
        char buf[16] = {};
        auto read = [&](std::size_t n, bool more) ->
            core::string_view
        {
            auto rs = fsource.read(
                capy::mutable_buffer{ buf, n });
            BOOST_TEST_EQ(rs.finished, !more);
            BOOST_TEST(!rs.ec);
            return { buf, rs.bytes };
        };
        BOOST_TEST_EQ(read(6, true), "Hello,");
        BOOST_TEST_EQ(read(1, true), " ");
        BOOST_TEST_EQ(read(6, true), "World!");
        BOOST_TEST_EQ(read(1, false), "");
    }

    void
    testBoundedRead()
    {
        temp_path path;
        write_file(path, "Hello, World!");
        capy::file f;
        system::error_code ec;
        f.open(path, capy::file_mode::read, ec);
        file_source fsource(
            std::move(f),
            6); // Bounded to 6 bytes

        {
            char buf[5] = {};
            auto rs = fsource.read(
                capy::make_buffer(buf));
            BOOST_TEST_EQ(rs.bytes, 5);
            BOOST_TEST(!rs.ec);
            BOOST_TEST(!rs.finished);
            BOOST_TEST_EQ(
                core::string_view(buf, 5),
                "Hello");
        }

        {
            char buf[5] = {};
            auto rs = fsource.read(
                capy::make_buffer(buf));
            BOOST_TEST_EQ(rs.bytes, 1);
            BOOST_TEST(!rs.ec);
            BOOST_TEST(rs.finished);
            BOOST_TEST_EQ(
                core::string_view(buf, 1),
                ",");
        }
    }

    void
    run()
    {
        testReportErros();
        testRead();
        testBoundedRead();
    }
};

TEST_SUITE(
    file_source_test,
    "boost.http.file_source");

} // http
} // boost
