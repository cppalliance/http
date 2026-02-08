//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_METADATA_HPP
#define BOOST_HTTP_METADATA_HPP

#include <boost/http/detail/config.hpp>
#include <boost/system/error_code.hpp>
#include <cstdint>
#include <cstdlib>

namespace boost {
namespace http {

/** Identifies the payload type of a message.
*/
enum class payload
{
    /** This message has no payload.
    */
    none,

    /** The payload is unknown due to errors.
    */
    error,

    /** This message has a known payload size.
    */
    size,

    /** This message contains a chunked payload.
    */
    chunked,

    /** The payload for this message continues until EOF.
    */
    to_eof
};

/** Standard content-codings for HTTP message bodies.
*/
enum class content_coding
{
    unknown,
    br,
    compress,
    dcb,
    dcz,
    deflate,
    gzip,
    identity,
    zstd,
};

/** Metadata about a request or response.
*/
struct metadata
{
    /** Metadata for the Connection field.
    */
    struct connection_t
    {
        /** Error status of Connection.
        */
        system::error_code ec;

        /** The total number of fields.
        */
        std::size_t count = 0;

        /** true if a close token is present.
        */
        bool close = false;

        /** true if a keep-alive token is present.
        */
        bool keep_alive = false;

        /** true if an upgrade token is present.
        */
        bool upgrade = false;

    #ifdef BOOST_HTTP_AGGREGATE_WORKAROUND
        constexpr
        connection_t() = default;

        constexpr
        connection_t(
            system::error_code ec_,
            std::size_t count_,
            bool close_,
            bool keep_alive_,
            bool upgrade_) noexcept
            : ec(ec_)
            , count(count_)
            , close(close_)
            , keep_alive(keep_alive_)
            , upgrade(upgrade_)
        {
        }
    #endif
    };

    //--------------------------------------------

    /** Metadata for the Content-Encoding field.
    */
    struct content_encoding_t
    {
        /** Error status of Content-Encoding.
        */
        system::error_code ec;

        /** The total number of fields.
        */
        std::size_t count = 0;

        /** The body encoding.
        */
        content_coding coding =
            content_coding::identity;

    #ifdef BOOST_HTTP_AGGREGATE_WORKAROUND
        constexpr
        content_encoding_t() = default;

        constexpr
        content_encoding_t(
            system::error_code ec_,
            std::size_t count_,
            content_coding coding_) noexcept
            : ec(ec_)
            , count(count_)
            , coding(coding_)
        {
        }
    #endif
    };

    //--------------------------------------------

    /** Metadata for the Content-Length field.
    */
    struct content_length_t
    {
        /** Error status of Content-Length.
        */
        system::error_code ec;

        /** The total number of fields.
        */
        std::size_t count = 0;

        /** The value as an integer.

            This is only valid when ec does
            not hold a failure, and when
            count is greater than zero.
        */
        std::uint64_t value = 0;

    #ifdef BOOST_HTTP_AGGREGATE_WORKAROUND
        constexpr
        content_length_t() = default;

        constexpr
        content_length_t(
            system::error_code ec_,
            std::size_t count_,
            std::uint64_t value_) noexcept
            : ec(ec_)
            , count(count_)
            , value(value_)
        {
        }
    #endif
    };

    //--------------------------------------------

    /** Metadata for the Expect field.
    */
    struct expect_t
    {
        /** Error status of Expect.
        */
        system::error_code ec;

        /** The total number of fields.
        */
        std::size_t count = 0;

        /** True if Expect is 100-continue.
        */
        bool is_100_continue = false;

    #ifdef BOOST_HTTP_AGGREGATE_WORKAROUND
        constexpr
        expect_t() = default;

        constexpr
        expect_t(
            system::error_code ec_,
            std::size_t count_,
            bool is_100_continue_) noexcept
            : ec(ec_)
            , count(count_)
            , is_100_continue(is_100_continue_)
        {
        }
    #endif
    };

    //--------------------------------------------

    /** Metadata for the Transfer-Encoding field.
    */
    struct transfer_encoding_t
    {
        /** Error status of Content-Length.
        */
        system::error_code ec;

        /** The total number of fields.
        */
        std::size_t count = 0;

        /** True if valid and chunked is specified last.
        */
        bool is_chunked = false;

    #ifdef BOOST_HTTP_AGGREGATE_WORKAROUND
        constexpr
        transfer_encoding_t() = default;

        constexpr
        transfer_encoding_t(
            system::error_code ec_,
            std::size_t count_,
            bool is_chunked_) noexcept
            : ec(ec_)
            , count(count_)
            , is_chunked(is_chunked_)
        {
        }
    #endif
    };

    //--------------------------------------------

    /** Metadata for Upgrade field.
    */
    struct upgrade_t
    {
        /** Error status of Upgrade.
        */
        system::error_code ec;

        /** The total number of fields.
        */
        std::size_t count = 0;

        /** True if websocket appears at least once.
        */
        bool websocket = false;

    #ifdef BOOST_HTTP_AGGREGATE_WORKAROUND
        constexpr
        upgrade_t() = default;

        constexpr
        upgrade_t(
            system::error_code ec_,
            std::size_t count_,
            bool websocket_) noexcept
            : ec(ec_)
            , count(count_)
            , websocket(websocket_)
        {
        }
    #endif
    };

    //--------------------------------------------

    /** True if payload is manually specified.

        This flag is used to allow the caller
        to resolve problems with non-compliant
        values for Content-Length.
    */
    bool payload_override = false;

    /** The type of payload.
    */
    http::payload payload =
        http::payload::none;

    /** The size of the payload if known.

        This is only valid when @ref payload
        equals @ref http::payload::size.
    */
    std::uint64_t payload_size = 0;

    //--------------------------------------------

    // header metadata

    /** Metadata for the Connection field.
    */
    connection_t connection;

    /** Metadata for the Content-Encoding field.
    */
    content_encoding_t content_encoding;

    /** Metadata for the Content-Length field.
    */
    content_length_t content_length;

    /** Metadata for the Expect field.
    */
    expect_t expect;

    /** Metadata for the Transfer-Encoding field.
    */
    transfer_encoding_t transfer_encoding;

    /** Metadata for the Upgrade field.
    */
    upgrade_t upgrade;

    //--------------------------------------------

    /** Constructor.
    */
    constexpr metadata() = default;
};

} // http
} // boost

#endif
