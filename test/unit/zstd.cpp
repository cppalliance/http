//
// Copyright (c) 2026 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/capy/ex/execution_context.hpp>
#include <boost/http/zstd.hpp>

#include "test_helpers.hpp"

#include <algorithm>
#include <string>

namespace boost {
namespace http {

class test_context : public capy::execution_context
{
public:
    ~test_context()
    {
        shutdown();
        destroy();
    }
};

struct zstd_test
{
    void
    test_error_code()
    {
        system::error_code ec = zstd::error::no_error;
        BOOST_TEST(! ec);
        BOOST_TEST(! ec.failed());
        BOOST_TEST_EQ(std::string(ec.category().name()), std::string("boost.http.zstd"));

        ec = zstd::error::corruption_detected;
        BOOST_TEST(ec.failed());
        BOOST_TEST(ec == zstd::error::corruption_detected);
        BOOST_TEST_EQ(ec.message(), "corruption_detected");

        ec = static_cast<zstd::error>(9999);
        BOOST_TEST(ec.failed());
        BOOST_TEST_EQ(ec.message(), "unknown");

        std::error_code sec = zstd::error::memory_allocation;
        BOOST_TEST(sec);
    }

#ifdef BOOST_HTTP_HAS_ZSTD
    static
    std::string
    sample_text()
    {
        std::string s;
        for(int i = 0; i < 100; ++i)
            s += "Hello, World! This is a test of zstd compression. ";
        return s;
    }

    void
    test_install()
    {
        test_context ctx;
        auto& csvc = zstd::install_compress_service(ctx);
        auto& dsvc = zstd::install_decompress_service(ctx);

        BOOST_TEST(ctx.find_service<zstd::compress_service>() == &csvc);
        BOOST_TEST(ctx.find_service<zstd::decompress_service>() == &dsvc);

        BOOST_TEST_GE(csvc.version_number(), 10400u);
        BOOST_TEST_EQ(csvc.version_number(), dsvc.version_number());
        BOOST_TEST_EQ(std::string(csvc.version_string()), std::string(dsvc.version_string()));

        BOOST_TEST_LT(csvc.min_level(), 0);
        BOOST_TEST_GE(csvc.max_level(), 19);
        BOOST_TEST_GE(csvc.default_level(), csvc.min_level());
        BOOST_TEST_LE(csvc.default_level(), csvc.max_level());

        BOOST_TEST_GT(csvc.stream_in_size(), 0u);
        BOOST_TEST_GT(csvc.stream_out_size(), 0u);
        BOOST_TEST_GT(dsvc.stream_in_size(), 0u);
        BOOST_TEST_GT(dsvc.stream_out_size(), 0u);
    }

    void
    test_one_shot()
    {
        test_context ctx;
        auto& csvc = zstd::install_compress_service(ctx);
        auto& dsvc = zstd::install_decompress_service(ctx);

        auto const input = sample_text();

        auto const bound = csvc.compress_bound(input.size());
        BOOST_TEST(! csvc.is_error(bound));
        BOOST_TEST_GE(bound, input.size());

        std::string compressed(bound, '\0');
        auto const n = csvc.compress(
            &compressed[0], compressed.size(),
            input.data(), input.size(),
            csvc.default_level());
        if(! BOOST_TEST(! csvc.is_error(n)))
            return;
        compressed.resize(n);
        BOOST_TEST_GT(compressed.size(), 0u);
        BOOST_TEST_LT(compressed.size(), input.size());

        BOOST_TEST_EQ(
            dsvc.get_frame_content_size(compressed.data(), compressed.size()),
            input.size());
        BOOST_TEST_EQ(
            dsvc.find_frame_compressed_size(compressed.data(), compressed.size()),
            compressed.size());
        BOOST_TEST_EQ(
            dsvc.get_dict_id_from_frame(compressed.data(), compressed.size()),
            0u);

        std::string output(input.size(), '\0');
        auto const m = dsvc.decompress(
            &output[0], output.size(),
            compressed.data(), compressed.size());
        if(! BOOST_TEST(! dsvc.is_error(m)))
            return;
        BOOST_TEST_EQ(m, input.size());
        BOOST_TEST(output == input);

        // a destination which is too small is an error
        std::string small(input.size() / 2, '\0');
        auto const r = dsvc.decompress(
            &small[0], small.size(),
            compressed.data(), compressed.size());
        BOOST_TEST(dsvc.is_error(r));
        BOOST_TEST(dsvc.get_error_code(r) == zstd::error::dst_size_too_small);
    }

    void
    test_stream()
    {
        test_context ctx;
        auto& csvc = zstd::install_compress_service(ctx);
        auto& dsvc = zstd::install_decompress_service(ctx);

        auto const input = sample_text();

        auto* cctx = csvc.create_cctx();
        if(! BOOST_TEST(cctx != nullptr))
            return;
        BOOST_TEST_GT(csvc.sizeof_cctx(cctx), 0u);
        BOOST_TEST(! csvc.is_error(csvc.set_parameter(
            cctx, zstd::c_parameter::compression_level, 5)));
        BOOST_TEST(! csvc.is_error(csvc.set_parameter(
            cctx, zstd::c_parameter::checksum_flag, 1)));
        BOOST_TEST(! csvc.is_error(csvc.set_pledged_src_size(
            cctx, input.size())));

        // Drive the compressor with a tiny output buffer
        // so that every directive needs multiple calls.
        std::string compressed;
        auto const drive = [&](
            zstd::in_buffer& in,
            zstd::end_directive op)
        {
            char buf[64];
            for(int i = 0; i < 10000; ++i)
            {
                zstd::out_buffer out{ buf, sizeof(buf), 0 };
                auto const rs = csvc.compress_stream(cctx, out, in, op);
                if(! BOOST_TEST(! csvc.is_error(rs)))
                    return false;
                compressed.append(buf, out.pos);
                if(op == zstd::end_directive::continue_)
                {
                    if(in.pos == in.size)
                        return true;
                }
                else if(rs == 0)
                {
                    return true;
                }
            }
            BOOST_TEST_FAIL();
            return false;
        };

        auto const half = input.size() / 2;
        zstd::in_buffer in1{ input.data(), half, 0 };
        if(! drive(in1, zstd::end_directive::continue_))
            return;
        zstd::in_buffer empty{ nullptr, 0, 0 };
        if(! drive(empty, zstd::end_directive::flush))
            return;
        // flushing emits a decodable block
        BOOST_TEST_GT(compressed.size(), 0u);
        zstd::in_buffer in2{ input.data() + half, input.size() - half, 0 };
        if(! drive(in2, zstd::end_directive::end))
            return;
        BOOST_TEST_LT(compressed.size(), input.size());
        BOOST_TEST_EQ(
            dsvc.get_frame_content_size(compressed.data(), compressed.size()),
            input.size());

        // the context can be reused for another frame
        BOOST_TEST(! csvc.is_error(csvc.reset(
            cctx, zstd::reset_directive::session_only)));
        {
            std::string second(csvc.compress_bound(input.size()), '\0');
            auto const n = csvc.compress2(
                cctx, &second[0], second.size(),
                input.data(), input.size());
            BOOST_TEST(! csvc.is_error(n));
            BOOST_TEST_GT(n, 0u);
        }
        BOOST_TEST(! csvc.is_error(csvc.free_cctx(cctx)));

        // decompress with a tiny output buffer
        auto* dctx = dsvc.create_dctx();
        if(! BOOST_TEST(dctx != nullptr))
            return;
        BOOST_TEST_GT(dsvc.sizeof_dctx(dctx), 0u);
        BOOST_TEST(! dsvc.is_error(dsvc.set_parameter(
            dctx, zstd::d_parameter::window_log_max, 27)));
        std::string output;
        {
            zstd::in_buffer in{ compressed.data(), compressed.size(), 0 };
            char buf[64];
            for(int i = 0;; ++i)
            {
                if(! BOOST_TEST_LT(i, 10000))
                    return;
                zstd::out_buffer out{ buf, sizeof(buf), 0 };
                auto const rs = dsvc.decompress_stream(dctx, out, in);
                if(! BOOST_TEST(! dsvc.is_error(rs)))
                    return;
                output.append(buf, out.pos);
                if(rs == 0)
                    break;
            }
            BOOST_TEST_EQ(in.pos, in.size);
        }
        BOOST_TEST(! dsvc.is_error(dsvc.free_dctx(dctx)));
        BOOST_TEST(output == input);
    }

    void
    test_dictionary()
    {
        test_context ctx;
        auto& csvc = zstd::install_compress_service(ctx);
        auto& dsvc = zstd::install_decompress_service(ctx);

        // A raw content dictionary; the input starts with it
        std::string const dict =
            "Hello, World! This is a test of zstd compression. ";
        auto const input = sample_text();

        // raw content is not a conformant dictionary
        BOOST_TEST_EQ(
            csvc.get_dict_id_from_dict(dict.data(), dict.size()), 0u);
        BOOST_TEST_EQ(
            dsvc.get_dict_id_from_dict(dict.data(), dict.size()), 0u);

        // compress with a digested dictionary
        auto* cd = csvc.create_cdict(dict.data(), dict.size(), 3);
        if(! BOOST_TEST(cd != nullptr))
            return;
        BOOST_TEST_GT(csvc.sizeof_cdict(cd), 0u);

        auto* cctx = csvc.create_cctx();
        if(! BOOST_TEST(cctx != nullptr))
            return;
        BOOST_TEST(! csvc.is_error(csvc.ref_cdict(cctx, cd)));
        std::string compressed(csvc.compress_bound(input.size()), '\0');
        auto n = csvc.compress2(
            cctx, &compressed[0], compressed.size(),
            input.data(), input.size());
        if(! BOOST_TEST(! csvc.is_error(n)))
            return;
        compressed.resize(n);
        BOOST_TEST(! csvc.is_error(csvc.ref_cdict(cctx, nullptr)));
        BOOST_TEST(! csvc.is_error(csvc.free_cdict(cd)));

        std::string output(input.size(), '\0');
        auto* dctx = dsvc.create_dctx();
        if(! BOOST_TEST(dctx != nullptr))
            return;

        // without the dictionary, decoding fails
        auto m = dsvc.decompress_dctx(
            dctx, &output[0], output.size(),
            compressed.data(), compressed.size());
        BOOST_TEST(dsvc.is_error(m));
        BOOST_TEST(! dsvc.is_error(dsvc.reset(
            dctx, zstd::reset_directive::session_and_parameters)));

        // with a digested dictionary
        auto* dd = dsvc.create_ddict(dict.data(), dict.size());
        if(! BOOST_TEST(dd != nullptr))
            return;
        BOOST_TEST_GT(dsvc.sizeof_ddict(dd), 0u);
        BOOST_TEST(! dsvc.is_error(dsvc.ref_ddict(dctx, dd)));
        m = dsvc.decompress_dctx(
            dctx, &output[0], output.size(),
            compressed.data(), compressed.size());
        BOOST_TEST(! dsvc.is_error(m));
        BOOST_TEST_EQ(m, input.size());
        BOOST_TEST(output == input);
        BOOST_TEST(! dsvc.is_error(dsvc.ref_ddict(dctx, nullptr)));
        BOOST_TEST(! dsvc.is_error(dsvc.free_ddict(dd)));

        // with a loaded dictionary
        std::fill(output.begin(), output.end(), '\0');
        BOOST_TEST(! dsvc.is_error(dsvc.load_dictionary(
            dctx, dict.data(), dict.size())));
        m = dsvc.decompress_dctx(
            dctx, &output[0], output.size(),
            compressed.data(), compressed.size());
        BOOST_TEST(! dsvc.is_error(m));
        BOOST_TEST_EQ(m, input.size());
        BOOST_TEST(output == input);
        BOOST_TEST(! dsvc.is_error(dsvc.load_dictionary(dctx, nullptr, 0)));

        // compress with a loaded dictionary
        BOOST_TEST(! csvc.is_error(csvc.load_dictionary(
            cctx, dict.data(), dict.size())));
        compressed.assign(csvc.compress_bound(input.size()), '\0');
        n = csvc.compress2(
            cctx, &compressed[0], compressed.size(),
            input.data(), input.size());
        if(! BOOST_TEST(! csvc.is_error(n)))
            return;
        compressed.resize(n);
        BOOST_TEST(! csvc.is_error(csvc.load_dictionary(cctx, nullptr, 0)));

        std::fill(output.begin(), output.end(), '\0');
        BOOST_TEST(! dsvc.is_error(dsvc.load_dictionary(
            dctx, dict.data(), dict.size())));
        m = dsvc.decompress_dctx(
            dctx, &output[0], output.size(),
            compressed.data(), compressed.size());
        BOOST_TEST(! dsvc.is_error(m));
        BOOST_TEST_EQ(m, input.size());
        BOOST_TEST(output == input);
        BOOST_TEST(! dsvc.is_error(dsvc.load_dictionary(dctx, nullptr, 0)));

        // with a prefix on both sides
        BOOST_TEST(! csvc.is_error(csvc.ref_prefix(
            cctx, dict.data(), dict.size())));
        compressed.assign(csvc.compress_bound(input.size()), '\0');
        n = csvc.compress2(
            cctx, &compressed[0], compressed.size(),
            input.data(), input.size());
        if(! BOOST_TEST(! csvc.is_error(n)))
            return;
        compressed.resize(n);
        BOOST_TEST(! csvc.is_error(csvc.free_cctx(cctx)));

        std::fill(output.begin(), output.end(), '\0');
        BOOST_TEST(! dsvc.is_error(dsvc.ref_prefix(
            dctx, dict.data(), dict.size())));
        m = dsvc.decompress_dctx(
            dctx, &output[0], output.size(),
            compressed.data(), compressed.size());
        BOOST_TEST(! dsvc.is_error(m));
        BOOST_TEST_EQ(m, input.size());
        BOOST_TEST(output == input);
        BOOST_TEST(! dsvc.is_error(dsvc.free_dctx(dctx)));
    }

    void
    test_errors()
    {
        test_context ctx;
        auto& csvc = zstd::install_compress_service(ctx);
        auto& dsvc = zstd::install_decompress_service(ctx);

        // invalid frame
        std::string const garbage = "this is definitely not a zstd frame";
        std::string output(64, '\0');
        auto const rs = dsvc.decompress(
            &output[0], output.size(),
            garbage.data(), garbage.size());
        BOOST_TEST(dsvc.is_error(rs));
        BOOST_TEST(dsvc.get_error_code(rs) == zstd::error::prefix_unknown);
        BOOST_TEST(dsvc.get_error_name(rs) != nullptr);
        BOOST_TEST_EQ(
            std::string(dsvc.error_string(zstd::error::prefix_unknown)),
            std::string(dsvc.get_error_name(rs)));
        BOOST_TEST_EQ(
            std::string(csvc.error_string(zstd::error::prefix_unknown)),
            std::string(dsvc.error_string(zstd::error::prefix_unknown)));
        BOOST_TEST_EQ(
            dsvc.get_frame_content_size(garbage.data(), garbage.size()),
            zstd::content_size_error);
        BOOST_TEST(dsvc.is_error(
            dsvc.find_frame_compressed_size(garbage.data(), garbage.size())));

        // the error code converts to a system::error_code
        system::error_code ec = dsvc.get_error_code(rs);
        BOOST_TEST(ec.failed());
        BOOST_TEST_EQ(ec.message(), "prefix_unknown");

        // successful results are not errors
        BOOST_TEST(! csvc.is_error(0));
        BOOST_TEST(csvc.get_error_code(0) == zstd::error::no_error);
        BOOST_TEST(! csvc.is_error(12345));
        BOOST_TEST(! dsvc.is_error(0));
        BOOST_TEST(dsvc.get_error_code(12345) == zstd::error::no_error);

        // parameter bounds
        auto const cb = csvc.param_bounds(zstd::c_parameter::compression_level);
        BOOST_TEST(! csvc.is_error(cb.error));
        BOOST_TEST_EQ(cb.lower_bound, csvc.min_level());
        BOOST_TEST_EQ(cb.upper_bound, csvc.max_level());

        auto const db = dsvc.param_bounds(zstd::d_parameter::window_log_max);
        BOOST_TEST(! dsvc.is_error(db.error));
        BOOST_TEST_LT(db.lower_bound, db.upper_bound);

        // out of bound parameter
        auto* cctx = csvc.create_cctx();
        if(! BOOST_TEST(cctx != nullptr))
            return;
        auto const pr = csvc.set_parameter(
            cctx, zstd::c_parameter::window_log, 1);
        BOOST_TEST(csvc.is_error(pr));
        BOOST_TEST(csvc.get_error_code(pr) ==
            zstd::error::parameter_out_of_bound);
        BOOST_TEST(! csvc.is_error(csvc.free_cctx(cctx)));

        // frames larger than the window limit are
        // rejected in streaming mode
        {
            auto const input = sample_text();
            std::string compressed(csvc.compress_bound(input.size()), '\0');
            auto const n = csvc.compress(
                &compressed[0], compressed.size(),
                input.data(), input.size(),
                csvc.default_level());
            if(! BOOST_TEST(! csvc.is_error(n)))
                return;
            compressed.resize(n);

            auto* dctx = dsvc.create_dctx();
            if(! BOOST_TEST(dctx != nullptr))
                return;
            BOOST_TEST(! dsvc.is_error(dsvc.set_parameter(
                dctx, zstd::d_parameter::window_log_max, 10)));
            zstd::in_buffer in{ compressed.data(), compressed.size(), 0 };
            zstd::out_buffer out{ &output[0], output.size(), 0 };
            auto const sr = dsvc.decompress_stream(dctx, out, in);
            BOOST_TEST(dsvc.is_error(sr));
            BOOST_TEST(dsvc.get_error_code(sr) ==
                zstd::error::frame_parameter_window_too_large);
            BOOST_TEST(! dsvc.is_error(dsvc.free_dctx(dctx)));
        }

        // null pointers are accepted by the free functions
        BOOST_TEST(! csvc.is_error(csvc.free_cctx(nullptr)));
        BOOST_TEST(! csvc.is_error(csvc.free_cdict(nullptr)));
        BOOST_TEST(! dsvc.is_error(dsvc.free_dctx(nullptr)));
        BOOST_TEST(! dsvc.is_error(dsvc.free_ddict(nullptr)));
    }
#endif

    void
    run()
    {
        test_error_code();
    #ifdef BOOST_HTTP_HAS_ZSTD
        test_install();
        test_one_shot();
        test_stream();
        test_dictionary();
        test_errors();
    #endif
    }
};

TEST_SUITE(zstd_test, "boost.http.zstd");

} // namespace http
} // namespace boost
