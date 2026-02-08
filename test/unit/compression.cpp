//
// Copyright (c) 2024 Christian Mazakas
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#if 0

#include <boost/capy/ex/execution_context.hpp>
#include <boost/core/detail/string_view.hpp>

#include <boost/http/zlib.hpp>

#ifdef BOOST_HTTP_HAS_BROTLI
#include <boost/http/brotli.hpp>
#endif

#include "test_helpers.hpp"

#include <string>
#include <vector>

namespace boost {
namespace http {

// Test execution_context that properly shuts down and destroys services
class test_context : public capy::execution_context
{
public:
    ~test_context()
    {
        shutdown();
        destroy();
    }
};

struct compression_test
{
    void
    test_zlib_deflate_inflate()
    {
        test_context ctx;

        auto& deflate_svc = zlib::install_deflate_service(ctx);
        auto& inflate_svc = zlib::install_inflate_service(ctx);

        // Test basic compression/decompression
        // Use repeated text to ensure it compresses well
        std::string input;
        for(int i = 0; i < 100; ++i)
            input += "Hello, World! This is a test of zlib compression. ";
        
        std::vector<unsigned char> compressed(input.size() * 2);
        std::vector<unsigned char> decompressed(input.size() + 1);

        // Compress
        zlib::stream zs_deflate{};
        zs_deflate.zalloc = nullptr;
        zs_deflate.zfree = nullptr;
        zs_deflate.opaque = nullptr;

        int ret = deflate_svc.init(zs_deflate, zlib::default_compression);
        BOOST_TEST_EQ(ret, static_cast<int>(zlib::error::ok));

        zs_deflate.next_in = reinterpret_cast<unsigned char*>(const_cast<char*>(input.data()));
        zs_deflate.avail_in = static_cast<unsigned>(input.size());
        zs_deflate.next_out = compressed.data();
        zs_deflate.avail_out = static_cast<unsigned>(compressed.size());

        ret = deflate_svc.deflate(zs_deflate, zlib::finish);
        BOOST_TEST_EQ(ret, static_cast<int>(zlib::error::stream_end));

        std::size_t compressed_size = zs_deflate.total_out;
        deflate_svc.deflate_end(zs_deflate);

        BOOST_TEST_GT(compressed_size, 0u);
        BOOST_TEST_LT(compressed_size, input.size());

        // Decompress
        zlib::stream zs_inflate{};
        zs_inflate.zalloc = nullptr;
        zs_inflate.zfree = nullptr;
        zs_inflate.opaque = nullptr;

        ret = inflate_svc.init(zs_inflate);
        BOOST_TEST_EQ(ret, static_cast<int>(zlib::error::ok));

        zs_inflate.next_in = compressed.data();
        zs_inflate.avail_in = static_cast<unsigned>(compressed_size);
        zs_inflate.next_out = decompressed.data();
        zs_inflate.avail_out = static_cast<unsigned>(decompressed.size());

        ret = inflate_svc.inflate(zs_inflate, zlib::finish);
        BOOST_TEST_EQ(ret, static_cast<int>(zlib::error::stream_end));

        std::size_t decompressed_size = zs_inflate.total_out;
        inflate_svc.inflate_end(zs_inflate);

        BOOST_TEST_EQ(decompressed_size, input.size());
        BOOST_TEST(std::string(reinterpret_cast<char*>(decompressed.data()), decompressed_size) == input);
    }

#ifdef BOOST_HTTP_HAS_BROTLI
    void
    test_brotli_encode_decode()
    {
        test_context ctx;
        
        auto& encode_svc = brotli::install_encode_service(ctx);
        auto& decode_svc = brotli::install_decode_service(ctx);

        // Test basic compression/decompression
        std::string input = "Hello, World! This is a test of brotli compression.";
        
        std::size_t max_compressed = encode_svc.max_compressed_size(input.size());
        std::vector<std::uint8_t> compressed(max_compressed);
        std::vector<std::uint8_t> decompressed(input.size() + 1);

        // Compress
        std::size_t compressed_size = compressed.size();
        bool success = encode_svc.compress(
            brotli::default_quality,
            brotli::default_window,
            brotli::encoder_mode::generic,
            input.size(),
            reinterpret_cast<const std::uint8_t*>(input.data()),
            &compressed_size,
            compressed.data());

        BOOST_TEST(success);
        BOOST_TEST_GT(compressed_size, 0u);

        // Decompress
        std::size_t decompressed_size = decompressed.size();
        auto result = decode_svc.decompress(
            compressed_size,
            compressed.data(),
            &decompressed_size,
            decompressed.data());

        BOOST_TEST_EQ(static_cast<int>(result), static_cast<int>(brotli::decoder_result::success));
        BOOST_TEST_EQ(decompressed_size, input.size());
        BOOST_TEST(std::string(reinterpret_cast<char*>(decompressed.data()), decompressed_size) == input);
    }
#endif

    void
    test_multiple_contexts()
    {
        // Test that multiple contexts can have their own services
        test_context ctx1;
        test_context ctx2;

        auto& deflate1 = zlib::install_deflate_service(ctx1);
        auto& deflate2 = zlib::install_deflate_service(ctx2);

        // Both services should work independently
        BOOST_TEST_NE(deflate1.version(), nullptr);
        BOOST_TEST_NE(deflate2.version(), nullptr);
        BOOST_TEST_EQ(std::string(deflate1.version()), std::string(deflate2.version()));
    }

    void run()
    {
        test_zlib_deflate_inflate();
#ifdef BOOST_HTTP_HAS_BROTLI
        test_brotli_encode_decode();
#endif
        test_multiple_contexts();
    }
};

TEST_SUITE(
    compression_test,
    "boost.http.compression");

} // namespace http
} // namespace boost

#endif
