//
// Copyright (c) 2021 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// Test that header file is self-contained.
#include <boost/http/metadata.hpp>

#include <boost/http/request.hpp>
#include <boost/http/response.hpp>

#include "test_helpers.hpp"

namespace boost {
namespace http {

// These tests check that the message
// containers correctly track changes
// in the metadata

struct metadata_test
{
    system::error_code const ok{};

    // make sure that subrange correctly
    // uses the count information in
    // the metadata
    void
    testSubrange()
    {
        auto const req = [](
            core::string_view s,
            field id,
            std::size_t n)
        {
            request req_(s);
            auto const r = req_.find_all(id);
            BOOST_TEST_EQ(std::distance(
                r.begin(), r.end()), n);
        };

        // Connection

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            field::connection, 0);

        req("GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "\r\n",
            field::connection, 1);

        req("GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "Accept: text/html\r\n"
            "Connection: keep-alive, upgrade\r\n"
            "\r\n",
            field::connection, 2);

        req("GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Connection: close\r\n"
            "Connection: keep-alive\r\n"
            "Accept: text/html\r\n"
            "Connection: upgrade\r\n"
            "\r\n",
            field::connection, 3);

        // Content-Length

        req("GET / HTTP/1.1\r\n"
            "Accept: text/html\r\n"
            "\r\n",
            field::content_length, 0);

        req("GET / HTTP/1.1\r\n"
            "Accept: text/html\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            field::content_length, 1);

        req("GET / HTTP/1.1\r\n"
            "Content-Length: 1\r\n"
            "Content-Length: 1\r\n"
            "Accept: text/html\r\n"
            "\r\n",
            field::content_length, 2);

        // Transfer-Encoding

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            field::transfer_encoding, 0);

        req("GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Accept: text/html\r\n"
            "\r\n",
            field::transfer_encoding, 1);

        req("GET / HTTP/1.1\r\n"
            "Transfer-Encoding: gzip\r\n"
            "Server: localhost\r\n"
            "Transfer-Encoding: compress\r\n"
            "Accept: text/html\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            field::transfer_encoding, 3);

        // Upgrade

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            field::upgrade, 0);

        req("GET / HTTP/1.1\r\n"
            "Upgrade: none\r\n"
            "\r\n",
            field::upgrade, 1);

        req("GET / HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "Server: localhost\r\n"
            "Upgrade: http/2\r\n"
            "Accept: text/html\r\n"
            "\r\n",
            field::upgrade, 2);
    }

    void
    testConnection()
    {
        auto const req = [](
            core::string_view s,
            void(*f)(message_base&),
            metadata::connection_t con)
        {
            {
                request req_(s);
                f(req_);
                auto const t =
                    req_.metadata().connection;
                BOOST_TEST_EQ(t.ec, con.ec);
                BOOST_TEST_EQ(t.count, con.count);
                BOOST_TEST_EQ(t.close, con.close);
                BOOST_TEST_EQ(t.keep_alive, con.keep_alive);
                BOOST_TEST_EQ(t.upgrade, con.upgrade);
            }
        };

        req("GET / HTTP/1.1\r\n"
            "Connection: /\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_connection, 1, false, false, false });

        req("GET / HTTP/1.1\r\n"
            "Connection: x\r\n"
            "Connection: /\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_connection, 2, false, false, false });

        req("GET / HTTP/1.1\r\n"
            "Connection: x, /\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_connection, 1, false, false, false });

        req("GET / HTTP/1.1\r\n"
            "Connection: /\r\n"
            "Connection: /\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_connection, 2, false, false, false });

        req("GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, true, false, false });

        req("GET / HTTP/1.1\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false, true, false });

        req("GET / HTTP/1.1\r\n"
            "Connection: upgrade\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false, false, true});

        req("GET / HTTP/1.1\r\n"
            "Connection: upgrade, close, keep-alive\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, true, true, true});

        req("GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Connection: upgrade, close, keep-alive\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, true, true, true});

        //----------------------------------------

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::connection, "close");
            },
            { ok, 1, true, false, false });

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::connection, "keep-alive");
            },
            { ok, 1, false, true, false});

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::connection, "upgrade");
            },
            { ok, 1, false, false, true});

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::connection, "close");
                f.append(field::connection, "keep-alive");
                f.append(field::connection, "upgrade");
            },
            { ok, 3, true, true, true});

        req("GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "Connection: upgrade\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::connection));
            },
            { ok, 2, false, true, true});

        // erase all
        req("GET / HTTP/1.1\r\n"
            "Connection: close, upgrade\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(field::connection);
            },
            { ok, 0, false, false, false});
    }

    void
    testContentEncoding()
    {
        auto const check = [](
            core::string_view s,
            void(*f)(message_base&),
            metadata::content_encoding_t ce)
        {
            request req(s);
            f(req);
            auto const t =
                req.metadata().content_encoding;
            BOOST_TEST_EQ(t.ec, ce.ec);
            BOOST_TEST_EQ(t.count, ce.count);
            BOOST_TEST_EQ(t.coding, ce.coding);
        };

        check(
            "GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 0, content_coding::identity });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Encoding: gzip\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, content_coding::gzip });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Encoding: gzip, deflate\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, content_coding::unknown });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Encoding: gzip\r\n"
            "Content-Encoding: deflate\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 2, content_coding::unknown });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Encoding: bad;\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_content_encoding, 1, content_coding::unknown});
    }

    void
    testContentLength()
    {
        auto const check = [](
            core::string_view s,
            void(*f)(message_base&),
            metadata::content_length_t clen)
        {
            request req(s);
            f(req);
            auto const t =
                req.metadata().content_length;
            BOOST_TEST_EQ(t.ec, clen.ec);
            BOOST_TEST_EQ(t.count, clen.count);
            BOOST_TEST_EQ(t.value, clen.value);
        };

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 0,0\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_content_length, 1, 0 });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 00\r\n"
            "Content-Length: 0,0\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_content_length, 2, 0 });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, 0 });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 2\r\n"
            "Content-Length: 2\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 2, 2 });

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 3\r\n"
            "Content-Length: 5\r\n"
            "\r\n",
            [](message_base&){},
            { error::multiple_content_length, 2, 0 });

        //----------------------------------------

        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 0\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(
                    field::content_length));
            },
            { ok, 1, 42 });

        // remove duplicate
        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 42\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(
                    field::content_length));
            },
            { ok, 1, 42 });

        // remove duplicate
        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 42\r\n"
            "Content-Length: 42\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(
                    field::content_length));
            },
            { ok, 2, 42 });

        // remove last
        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(
                    field::content_length));
            },
            { ok, 0, 0 });

        // erase all
        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 1\r\n"
            "Content-Length: 2\r\n"
            "Content-Length: 3\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(field::content_length);
            },
            { ok, 0, 0 });

        // erase all
        check(
            "GET / HTTP/1.1\r\n"
            "Content-Length: 2\r\n"
            "Content-Length: 2\r\n"
            "Content-Length: 2\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(field::content_length);
            },
            { ok, 0, 0 });
    }

    void
    testTransferEncoding()
    {
        auto const check = [](
            core::string_view s,
            void(*f)(message_base&),
            metadata::transfer_encoding_t te)
        {
            request req(s);
            f(req);
            auto const t =
                req.metadata().transfer_encoding;
            BOOST_TEST_EQ(t.ec, te.ec);
            BOOST_TEST_EQ(t.count, te.count);
            BOOST_TEST_EQ(t.is_chunked, te.is_chunked);
        };

        check(
            "GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 0, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, true });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: chunked, chunked\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_transfer_encoding, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_transfer_encoding, 2, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: chunked, compress\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_transfer_encoding, 1, false});

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n"
            "Transfer-Encoding: compress\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_transfer_encoding, 2, false});

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: chunked;a=b\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_transfer_encoding, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: deflate;a=b\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_transfer_encoding, 2, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: compress\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: deflate\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: gzip\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: custom;a=1\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false});

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: a,b,c\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false});

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: a,b,c\r\n"
            "Transfer-Encoding: x,y\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 2, false});

        //----------------------------------------

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: compress\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::transfer_encoding));
            },
            { ok, 0, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Transfer-Encoding: compress\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::transfer_encoding));
            },
            { ok, 1, true });

        check(
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Transfer-Encoding: compress\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::transfer_encoding));
            },
            { ok, 1, true });

        check(
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::transfer_encoding));
            },
            { ok, 0, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Transfer-Encoding: gzip\r\n"
            "Transfer-Encoding: compress\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(field::transfer_encoding);
            },
            { ok, 0, false });
    }

    void
    testUpgrade()
    {
        auto const check = [](
            core::string_view s,
            void(*f)(message_base&),
            metadata::upgrade_t te)
        {
            request req(s);
            f(req);
            auto const t =
                req.metadata().upgrade;
            BOOST_TEST_EQ(t.ec, te.ec);
            BOOST_TEST_EQ(t.count, te.count);
            BOOST_TEST_EQ(t.websocket, te.websocket);
        };

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, true });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: WEBSOCKET\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, true });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: /usr\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_upgrade, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: /usr\r\n"
            "Upgrade: websocket\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_upgrade, 2, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket, /usr\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_upgrade, 1, false });

        // HTTP/1.0
        check(
            "GET / HTTP/1.0\r\n"
            "Upgrade: websocket\r\n"
            "\r\n",
            [](message_base&){},
            { error::bad_upgrade, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: chaka\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "Upgrade: rick/morty\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 2, true });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket/2\r\n"
            "\r\n",
            [](message_base&){},
            { ok, 1, false });

        //----------------------------------------

        check(
            "GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::upgrade, "http/2");
            },
            { ok, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: chaka\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::upgrade));
            },
            { ok, 0, false });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::upgrade));
            },
            { ok, 0, false });

        check(
            "GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::upgrade, "websocket");
                f.append(field::upgrade, "chaka");
                f.erase(f.find(field::upgrade));
            },
            { ok, 1, false });

        check(
            "GET / HTTP/1.1\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::upgrade, "chaka");
                f.append(field::upgrade, "websocket");
                f.erase(f.find(field::upgrade));
            },
            { ok, 1, true });

        check(
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "Upgrade: http/2\r\n"
            "Upgrade: chaka\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(field::upgrade);
            },
            { ok, 0, false });
    }

    void
    testOtherFields()
    {
        auto const check = [](
            core::string_view s,
            void(*f)(message_base&),
            core::string_view s1)
        {
            request req(s);
            f(req);
            BOOST_TEST(req.buffer() == s1);
        };

        check(
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.append(field::accept, "text/html");
            },
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "\r\n");

        check(
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(f.find(field::accept));
            },
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "\r\n");

        check(
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "Set-Cookie: 1\r\n"
            "Set-Cookie: 2\r\n"
            "Set-Cookie: 3\r\n"
            "\r\n",
            [](message_base& f)
            {
                f.erase(field::set_cookie);
            },
            "GET / HTTP/1.1\r\n"
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "\r\n");
    }

    void
    testFields()
    {
        auto const check = [](
            core::string_view s,
            void(*f)(fields_base&),
            core::string_view s1)
        {
            fields fld(s);
            f(fld);
            BOOST_TEST(
                fld.buffer() == s1);
        };

        check(
            "Server: localhost\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.append(field::accept, "text/html");
            },
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "\r\n");

        check(
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.erase(f.find(field::accept));
            },
            "Server: localhost\r\n"
            "\r\n");

        check(
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "Set-Cookie: 1\r\n"
            "Set-Cookie: 2\r\n"
            "Set-Cookie: 3\r\n"
            "\r\n",
            [](fields_base& f)
            {
                f.erase(field::set_cookie);
            },
            "Server: localhost\r\n"
            "Accept: text/html\r\n"
            "\r\n");
    }

    void
    testPayload()
    {
        auto const req = [](
            core::string_view s,
            void(*f)(fields_base&),
            payload v,
            std::uint64_t n = 0)
        {
            request req_(s);
            f(req_);
            BOOST_TEST_EQ(req_.payload(), v);
            if(req_.payload() == payload::size)
                BOOST_TEST_EQ(req_.payload_size(), n);
        };

        auto const res = [](
            core::string_view s,
            void(*f)(fields_base&),
            payload pay,
            std::uint64_t n = 0)
        {
            response res_(s);
            f(res_);
            BOOST_TEST_EQ(res_.payload(), pay);
            if(res_.payload() == payload::size)
                BOOST_TEST_EQ(res_.payload_size(), n);
        };

        req("GET / HTTP/1.1\r\n"
            "Content-Length: 0,0\r\n"
            "\r\n",
            [](fields_base&){},
            payload::error);

        req("GET / HTTP/1.1\r\n"
            "Transfer-Encoding: /rekt\r\n"
            "\r\n",
            [](fields_base&){},
            payload::error);

        req("GET / HTTP/1.1\r\n"
            "Content-Length: 42\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](fields_base&){},
            payload::error);

        req("GET / HTTP/1.1\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](fields_base&){},
            payload::size, 42);

        req("GET / HTTP/1.1\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            [](fields_base&){},
            payload::none);

        req("GET / HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](fields_base&){},
            payload::chunked);

        req("GET / HTTP/1.1\r\n"
            "\r\n",
            [](fields_base&){},
            payload::none);

        //----------------------------------------

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 0,0\r\n"
            "\r\n",
            [](fields_base&){},
            payload::error);

        res("HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: /rekt\r\n"
            "\r\n",
            [](fields_base&){},
            payload::error);

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 42\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](fields_base&){},
            payload::error);

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 42\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](fields_base&){},
            payload::error);

        res("HTTP/1.1 101 Switching Protocols\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](fields_base&){},
            payload::none);

        res("HTTP/1.1 204 No Content\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](fields_base&){},
            payload::none);

        res("HTTP/1.1 304 Not Modified\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](fields_base&){},
            payload::none);

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 42\r\n"
            "\r\n",
            [](fields_base&){},
            payload::size, 42);

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            [](fields_base&){},
            payload::none);

        res("HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n",
            [](fields_base&){},
            payload::chunked);

        res("HTTP/1.1 200 OK\r\n"
            "\r\n",
            [](fields_base&){},
            payload::to_eof);
    }

    void
    testKeepAlive()
    {
        auto const res = [](
            core::string_view s,
            void(*f)(fields_base&),
            bool keep_alive)
        {
            response res_(s);
            f(res_);
            BOOST_TEST_EQ(
                res_.keep_alive(), keep_alive);
        };

        auto const set = [](
            core::string_view s0,
            bool keep_alive,
            core::string_view s1)
        {
            request m(s0);
            m.set_keep_alive(keep_alive);
            BOOST_TEST_EQ(m.buffer(), s1);
            BOOST_TEST_EQ(
                m.keep_alive(), keep_alive);
        };

        // HTTP/1.0

        res("HTTP/1.0 200 OK\r\n"
            "\r\n",
            [](fields_base&){},
            false);

        // no Content-Length, requires EOF
        res("HTTP/1.0 200 OK\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            [](fields_base&){},
            false);

        res("HTTP/1.0 200 OK\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            [](fields_base&){},
            false);

        res("HTTP/1.0 200 OK\r\n"
            "Content-Length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            [](fields_base&){},
            true);

        // HTTP/1.1

        res("HTTP/1.1 200 OK\r\n"
            "\r\n",
            [](fields_base&){},
            false);

        res("HTTP/1.1 200 OK\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            [](fields_base&){},
            false);

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            [](fields_base&){},
            true);

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            [](fields_base&){},
            true);

        res("HTTP/1.1 200 OK\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n",
            [](fields_base&){},
            false);

        // error in payload
        res("HTTP/1.1 200 OK\r\n"
            "Connection: keep-alive\r\n"
            "Content-Length: 0,0\r\n"
            "\r\n",
            [](fields_base&){},
            false);

        //
        // set_keep_alive
        //

        // http/1.0

        set("GET / HTTP/1.0\r\n"
            "\r\n",
            false,
            "GET / HTTP/1.0\r\n"
            "\r\n");

        set("GET / HTTP/1.0\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n");

        set("GET / HTTP/1.0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            false,
            "GET / HTTP/1.0\r\n"
            "\r\n");

        set("GET / HTTP/1.0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n");

        // http/1.1

        set("GET / HTTP/1.1\r\n"
            "\r\n",
            false,
            "GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "\r\n");

        set("GET / HTTP/1.1\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.1\r\n"
            "\r\n");

        set("GET / HTTP/1.1\r\n"
            "\r\n",
            false,
            "GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "\r\n");

        set("GET / HTTP/1.1\r\n"
            "Connection: close\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.1\r\n"
            "\r\n");

        // erase_token

        set("GET / HTTP/1.1\r\n"
            "Connection: close, upgrade\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.1\r\n"
            "Connection: upgrade\r\n"
            "\r\n");

        set("GET / HTTP/1.1\r\n"
            "Connection: upgrade, close\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.1\r\n"
            "Connection: upgrade\r\n"
            "\r\n");

        set("GET / HTTP/1.1\r\n"
            "Connection: upgrade, close, keep_alive\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.1\r\n"
            "Connection: upgrade, keep_alive\r\n"
            "\r\n");

        // multiple fields

        set("GET / HTTP/1.1\r\n"
            "Connection: upgrade\r\n"
            "Connection: close\r\n"
            "Connection: keep_alive\r\n"
            "\r\n",
            true,
            "GET / HTTP/1.1\r\n"
            "Connection: upgrade\r\n"
            "Connection: keep_alive\r\n"
            "\r\n");
    }

    void
    run()
    {
        testSubrange();
        testConnection();
        testContentEncoding();
        testContentLength();
        testTransferEncoding();
        testUpgrade();
        testOtherFields();
        testFields();
        testPayload();
        testKeepAlive();
    }
};

TEST_SUITE(
    metadata_test,
    "boost.http.metadata");

} // http
} // boost
