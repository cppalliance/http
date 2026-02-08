//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include "src/rfc/detail/rules.hpp"
#include "src/rfc/detail/transfer_coding_rule.hpp"

#include <boost/http/detail/header.hpp>
#include <boost/http/field.hpp>
#include <boost/http/header_limits.hpp>
#include <boost/http/rfc/list_rule.hpp>
#include <boost/http/rfc/token_rule.hpp>
#include <boost/http/rfc/upgrade_rule.hpp>
#include <boost/assert.hpp>
#include <boost/assert/source_location.hpp>
#include <boost/static_assert.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/range_rule.hpp>
#include <boost/url/grammar/recycled.hpp>
#include <boost/url/grammar/unsigned_rule.hpp>

#include <utility>

namespace boost {
namespace http {
namespace detail {

//------------------------------------------------

auto
header::
entry::
operator+(
    std::size_t dv) const noexcept ->
        entry
{
    return {
        static_cast<
            offset_type>(np + dv),
        nn,
        static_cast<
            offset_type>(vp + dv),
        vn,
        id };
}

auto
header::
entry::
operator-(
    std::size_t dv) const noexcept ->
        entry
{
    return {
        static_cast<
            offset_type>(np - dv),
        nn,
        static_cast<
            offset_type>(vp - dv),
        vn,
        id };
}

//------------------------------------------------

constexpr field header::unknown_field;

//------------------------------------------------

constexpr
header::
header(fields_tag) noexcept
    : kind(detail::kind::fields)
    , cbuf("\r\n")
    , size(2)
    , fld{}
{
}

constexpr
header::
header(request_tag) noexcept
    : kind(detail::kind::request)
    , cbuf("GET / HTTP/1.1\r\n\r\n")
    , size(18)
    , prefix(16)
    , req{ 3, 1,
        http::method::get }
{
}

constexpr
header::
header(response_tag) noexcept
    : kind(detail::kind::response)
    , cbuf("HTTP/1.1 200 OK\r\n\r\n")
    , size(19)
    , prefix(17)
    , res{ 200,
        http::status::ok }
{
}

//------------------------------------------------

header const*
header::
get_default(detail::kind k) noexcept
{
    static constexpr header h[3] = {
        fields_tag{},
        request_tag{},
        response_tag{}};
    return &h[k];
}

header::
header(empty v) noexcept
    : kind(v.param)
{
}

header::
header(detail::kind k) noexcept
    : header(*get_default(k))
{
}

void
header::
swap(header& h) noexcept
{
    std::swap(cbuf, h.cbuf);
    std::swap(buf, h.buf);
    std::swap(cap, h.cap);
    std::swap(size, h.size);
    std::swap(count, h.count);
    std::swap(prefix, h.prefix);
    std::swap(version, h.version);
    std::swap(md, h.md);
    switch(kind)
    {
    default:
    case detail::kind::fields:
        break;
    case detail::kind::request:
        std::swap(
            req.method_len, h.req.method_len);
        std::swap(
            req.target_len, h.req.target_len);
        std::swap(req.method, h.req.method);
        break;
    case detail::kind::response:
        std::swap(
            res.status_int, h.res.status_int);
        std::swap(res.status, h.res.status);
        break;
    }
}

/*  References:

    6.3.  Persistence
    https://datatracker.ietf.org/doc/html/rfc7230#section-6.3
*/
bool
header::
keep_alive() const noexcept
{
    if(md.payload == payload::error)
        return false;
    if( version ==
        http::version::http_1_1)
    {
        if(md.connection.close)
            return false;
    }
    else
    {
        if(! md.connection.keep_alive)
            return false;
    }
    // can't use to_eof in requests
    BOOST_ASSERT(
        kind != detail::kind::request ||
        md.payload != payload::to_eof);
    if(md.payload == payload::to_eof)
        return false;
    return true;
}

//------------------------------------------------

// return total bytes needed
// to store message of `size`
// bytes and `count` fields.
std::size_t
header::
bytes_needed(
    std::size_t size,
    std::size_t count) noexcept
{
    // make sure `size` is big enough
    // to hold the largest default buffer:
    // "HTTP/1.1 200 OK\r\n\r\n"
    if(size < 19)
        size = 19;

    // align size up to alignof(entry)
    size = (size + alignof(entry) - 1) & ~(alignof(entry) - 1);

    return size + count * sizeof(entry);
}

std::size_t
header::
table_space(
    std::size_t count) noexcept
{
    return count *
        sizeof(header::entry);
}

std::size_t
header::
table_space() const noexcept
{
    return table_space(count);
}

auto
header::
tab() const noexcept ->
    table
{
    BOOST_ASSERT(cap > 0);
    BOOST_ASSERT(buf != nullptr);
    return table(buf + cap);
}

auto
header::
tab_() const noexcept ->
    entry*
{
    return reinterpret_cast<
        entry*>(buf + cap);
}

// return true if header cbuf is a default
bool
header::
is_default() const noexcept
{
    return buf == nullptr;
}

std::size_t
header::
find(
    field id) const noexcept
{
    if(count == 0)
        return 0;
    std::size_t i = 0;
    auto const* p = &tab()[0];
    while(i < count)
    {
        if(p->id == id)
            break;
        ++i;
        --p;
    }
    return i;
}

std::size_t
header::
find(
    core::string_view name) const noexcept
{
    if(count == 0)
        return 0;
    std::size_t i = 0;
    auto const* p = &tab()[0];
    while(i < count)
    {
        core::string_view s(
            cbuf + prefix + p->np,
            p->nn);
        if(grammar::ci_is_equal(s, name))
            break;
        ++i;
        --p;
    }
    return i;
}

void
header::
copy_table(
    void* dest,
    std::size_t n) const noexcept
{
    // When `n == 0`, cbuf + cap may have incorrect
    // alignment, which can trigger UB sanitizer.
    if(n == 0)
        return;

    std::memcpy(
        reinterpret_cast<
            entry*>(dest) - n,
        reinterpret_cast<
            entry const*>(
                cbuf + cap) - n,
        n * sizeof(entry));
}

void
header::
copy_table(
    void* dest) const noexcept
{
    copy_table(dest, count);
}

// assign all the members but
// preserve the allocated memory
void
header::
assign_to(
    header& dest) const noexcept
{
    auto const buf_ = dest.buf;
    auto const cbuf_ = dest.cbuf;
    auto const cap_ = dest.cap;
    dest = *this;
    dest.buf = buf_;
    dest.cbuf = cbuf_;
    dest.cap = cap_;
}

//------------------------------------------------
//
// Metadata
//
//------------------------------------------------

std::size_t
header::
maybe_count(
    field id) const noexcept
{
    if(kind == detail::kind::fields)
        return std::size_t(-1);
    switch(id)
    {
    case field::connection:
        return md.connection.count;
    case field::content_encoding:
        return md.content_encoding.count;
    case field::content_length:
        return md.content_length.count;
    case field::expect:
        return md.expect.count;
    case field::transfer_encoding:
        return md.transfer_encoding.count;
    case field::upgrade:
        return md.upgrade.count;
    default:
        break;
    }
    return std::size_t(-1);
}

bool
header::
is_special(
    field id) const noexcept
{
    if(kind == detail::kind::fields)
        return false;
    switch(id)
    {
    case field::connection:
    case field::content_encoding:
    case field::content_length:
    case field::expect:
    case field::transfer_encoding:
    case field::upgrade:
        return true;
    default:
        break;
    }
    return false;
}

//------------------------------------------------

// called when the start-line changes
void
header::
on_start_line()
{
    // items in both the request-line
    // and the status-line can affect
    // the payload, for example whether
    // or not EOF marks the end of the
    // payload.

    update_payload();
}

// called after a field is inserted
void
header::
on_insert(
    field id,
    core::string_view v)
{
    if(kind == detail::kind::fields)
        return;
    switch(id)
    {
    case field::content_encoding:
        return on_insert_content_encoding(v);
    case field::content_length:
        return on_insert_content_length(v);
    case field::connection:
        return on_insert_connection(v);
    case field::expect:
        return on_insert_expect(v);
    case field::transfer_encoding:
        return on_insert_transfer_encoding(v);
    case field::upgrade:
        return on_insert_upgrade(v);
    default:
        break;
    }
}

// called when one field is erased
void
header::
on_erase(field id)
{
    if(kind == detail::kind::fields)
        return;
    switch(id)
    {
    case field::connection:
        return on_erase_connection();
    case field::content_encoding:
        return on_erase_content_encoding();
    case field::content_length:
        return on_erase_content_length();
    case field::expect:
        return on_erase_expect();
    case field::transfer_encoding:
        return on_erase_transfer_encoding();
    case field::upgrade:
        return on_erase_upgrade();
    default:
        break;
    }
}

//------------------------------------------------

/*
    https://datatracker.ietf.org/doc/html/rfc7230#section-6.1
*/
void
header::
on_insert_connection(
    core::string_view v)
{
    ++md.connection.count;
    if(md.connection.ec)
        return;
    auto rv = grammar::parse(
        v, list_rule(token_rule, 1));
    if(! rv)
    {
        md.connection.ec =
            BOOST_HTTP_ERR(
                error::bad_connection);
        return;
    }
    md.connection.ec = {};
    for(auto t : *rv)
    {
        if(grammar::ci_is_equal(
                t, "close"))
            md.connection.close = true;
        else if(grammar::ci_is_equal(
                t, "keep-alive"))
            md.connection.keep_alive = true;
        else if(grammar::ci_is_equal(
                t, "upgrade"))
            md.connection.upgrade = true;
    }
}

void
header::
on_insert_content_length(
    core::string_view v)
{
    static
    constexpr
    grammar::unsigned_rule<
        std::uint64_t> num_rule{};

    ++md.content_length.count;
    if(md.content_length.ec)
        return;
    auto rv =
        grammar::parse(v, num_rule);
    if(! rv)
    {
        // parse failure
        md.content_length.ec =
            BOOST_HTTP_ERR(
            error::bad_content_length);
        md.content_length.value = 0;
        update_payload();
        return;
    }
    if(md.content_length.count == 1)
    {
        // one value
        md.content_length.ec = {};
        md.content_length.value = *rv;
        update_payload();
        return;
    }
    if(*rv == md.content_length.value)
    {
        // ok: duplicate value
        return;
    }
    // bad: different values
    md.content_length.ec =
        BOOST_HTTP_ERR(
            error::multiple_content_length);
    md.content_length.value = 0;
    update_payload();
}

void
header::
on_insert_expect(
    core::string_view v)
{
    ++md.expect.count;
    if(kind != detail::kind::request)
        return;
    if(md.expect.ec)
        return;
    // VFALCO Should we allow duplicate
    // Expect fields that have 100-continue?
    if( md.expect.count > 1 ||
        ! grammar::ci_is_equal(v,
            "100-continue"))
    {
        md.expect.ec =
            BOOST_HTTP_ERR(
                error::bad_expect);
        md.expect.is_100_continue = false;
        return;
    }
    md.expect.is_100_continue = true;
}

void
header::
on_insert_transfer_encoding(
    core::string_view v)
{
    ++md.transfer_encoding.count;
    if(md.transfer_encoding.ec)
        return;

    auto rv = grammar::parse(
        v, list_rule(transfer_coding_rule, 1));
    if(! rv)
    {
        // parse error
        goto error;
    }
    for(auto t : *rv)
    {
        if(! md.transfer_encoding.is_chunked)
        {
            if(t.id == transfer_coding_rule_t::chunked)
                md.transfer_encoding.is_chunked = true;
            continue;
        }
        if(t.id == transfer_coding_rule_t::chunked)
        {
            // chunked appears twice
            goto error;
        }
        // chunked must be last
        goto error;
    }
    update_payload();
    return;

error:
    md.transfer_encoding.ec =
        BOOST_HTTP_ERR(
            error::bad_transfer_encoding);
    md.transfer_encoding.is_chunked = false;
    update_payload();
}

void
header::
on_insert_content_encoding(
    core::string_view v)
{
    ++md.content_encoding.count;
    if(md.content_encoding.ec)
        return;

    auto rv = grammar::parse(
        v, list_rule(token_rule, 1));
    if(!rv)
    {
        md.content_encoding.ec =
            BOOST_HTTP_ERR(
                error::bad_content_encoding);
        md.content_encoding.coding =
            content_coding::unknown;
        return;
    }

    if(rv->size() > 1 || md.content_encoding.count > 1)
    {
        md.content_encoding.coding =
            content_coding::unknown;
        return;
    }

    if(grammar::ci_is_equal(
        *rv->begin(), "deflate"))
    {
        md.content_encoding.coding =
            content_coding::deflate;
    }
    else if(grammar::ci_is_equal(
        *rv->begin(), "gzip"))
    {
        md.content_encoding.coding =
            content_coding::gzip;
    }
    else if(grammar::ci_is_equal(
        *rv->begin(), "br"))
    {
        md.content_encoding.coding =
            content_coding::br;
    }
    else
    {
        md.content_encoding.coding =
            content_coding::unknown;
    }
}

void
header::
on_insert_upgrade(
    core::string_view v)
{
    ++md.upgrade.count;
    if(md.upgrade.ec)
        return;
    if( version !=
        http::version::http_1_1)
    {
        md.upgrade.ec =
            BOOST_HTTP_ERR(
                error::bad_upgrade);
        md.upgrade.websocket = false;
        return;
    }
    auto rv = grammar::parse(
        v, upgrade_rule);
    if(! rv)
    {
        md.upgrade.ec =
            BOOST_HTTP_ERR(
                error::bad_upgrade);
        md.upgrade.websocket = false;
        return;
    }
    if(! md.upgrade.websocket)
    {
        for(auto t : *rv)
        {
            if( grammar::ci_is_equal(
                    t.name, "websocket") &&
                t.version.empty())
            {
                md.upgrade.websocket = true;
                break;
            }
        }
    }
}

//------------------------------------------------

void
header::
on_erase_connection()
{
    BOOST_ASSERT(
        md.connection.count > 0);
    // reset and re-insert
    auto n = md.connection.count - 1;
    auto const p = cbuf + prefix;
    auto const* e = &tab()[0];
    md.connection = {};
    while(n > 0)
    {
        if(e->id == field::connection)
        {
            on_insert_connection(
                core::string_view(
                    p + e->vp, e->vn));
            --n;
        }
        --e;
    }
}

void
header::
on_erase_content_length()
{
    BOOST_ASSERT(
        md.content_length.count > 0);
    --md.content_length.count;
    if(md.content_length.count == 0)
    {
        // no Content-Length
        md.content_length = {};
        update_payload();
        return;
    }
    if(! md.content_length.ec)
    {
        // removing a duplicate value
        return;
    }
    // reset and re-insert
    auto n = md.content_length.count;
    auto const p = cbuf + prefix;
    auto const* e = &tab()[0];
    md.content_length = {};
    while(n > 0)
    {
        if(e->id == field::content_length)
        {
            on_insert_content_length(
                core::string_view(
                    p + e->vp, e->vn));
            --n;
        }
        --e;
    }
    update_payload();
}

void
header::
on_erase_expect()
{
    BOOST_ASSERT(
        md.expect.count > 0);
    --md.expect.count;
    if(kind != detail::kind::request)
        return;
    if(md.expect.count == 0)
    {
        // no Expect
        md.expect = {};
        return;
    }
    // VFALCO This should be uncommented
    // if we want to allow multiple Expect
    // fields with the value 100-continue
    /*
    if(! md.expect.ec)
        return;
    */
    // reset and re-insert
    auto n = md.expect.count;
    auto const p = cbuf + prefix;
    auto const* e = &tab()[0];
    md.expect = {};
    while(n > 0)
    {
        if(e->id == field::expect)
        {
            on_insert_expect(
                core::string_view(
                    p + e->vp, e->vn));
            --n;
        }
        --e;
    }
}

void
header::
on_erase_transfer_encoding()
{
    BOOST_ASSERT(
        md.transfer_encoding.count > 0);
    // reset and re-insert
    auto n = md.transfer_encoding.count - 1;
    auto const p = cbuf + prefix;
    auto const* e = &tab()[0];
    md.transfer_encoding = {};
    while(n > 0)
    {
        if(e->id == field::transfer_encoding)
        {
            on_insert_transfer_encoding(
                core::string_view(
                    p + e->vp, e->vn));
            --n;
        }
        --e;
    }
}

void
header::
on_erase_content_encoding()
{
    BOOST_ASSERT(
        md.content_encoding.count > 0);
    --md.content_encoding.count;
    if(md.content_encoding.count == 0)
    {
        // no Content-Encoding
        md.content_encoding = {};
        return;
    }
    // re-insert everything
    --md.content_encoding.count;
    // TODO
    // on_insert_content_encoding();
}

// called when Upgrade is erased
void
header::
on_erase_upgrade()
{
    BOOST_ASSERT(
        md.upgrade.count > 0);
    --md.upgrade.count;
    if(md.upgrade.count == 0)
    {
        // no Upgrade
        md.upgrade = {};
        return;
    }
    // reset and re-insert
    auto n = md.upgrade.count;
    auto const p = cbuf + prefix;
    auto const* e = &tab()[0];
    md.upgrade = {};
    while(n > 0)
    {
        if(e->id == field::upgrade)
            on_insert_upgrade(
                core::string_view(
                    p + e->vp, e->vn));
        --n;
        --e;
    }
}

//------------------------------------------------

// called when all fields with id are removed
void
header::
on_erase_all(
    field id)
{
    if(kind == detail::kind::fields)
        return;
    switch(id)
    {
    case field::connection:
        md.connection = {};
        return;

    case field::content_length:
        md.content_length = {};
        update_payload();
        return;

    case field::expect:
        md.expect = {};
        update_payload();
        return;

    case field::transfer_encoding:
        md.transfer_encoding = {};
        update_payload();
        return;

    case field::upgrade:
        md.upgrade = {};
        return;

    default:
        break;
    }
}

//------------------------------------------------

/*  References:

    3.3.  Message Body
    https://datatracker.ietf.org/doc/html/rfc7230#section-3.3

    3.3.1.  Transfer-Encoding
    https://datatracker.ietf.org/doc/html/rfc7230#section-3.3.1

    3.3.2.  Content-Length
    https://datatracker.ietf.org/doc/html/rfc7230#section-3.3.2
*/
void
header::
update_payload() noexcept
{
    BOOST_ASSERT(kind !=
        detail::kind::fields);
    if(md.payload_override)
    {
        // e.g. response to
        // a HEAD request
        return;
    }

/*  If there is an error in either Content-Length
    or Transfer-Encoding, then the payload is
    undefined. Clients should probably close the
    connection. Servers can send a Bad Request
    and avoid reading any payload bytes.
*/
    if(md.content_length.ec)
    {
        // invalid Content-Length
        md.payload = payload::error;
        md.payload_size = 0;
        return;
    }
    if(md.transfer_encoding.ec)
    {
        // invalid Transfer-Encoding
        md.payload = payload::error;
        md.payload_size = 0;
        return;
    }

/*  A sender MUST NOT send a Content-Length
    header field in any message that contains
    a Transfer-Encoding header field.
    https://datatracker.ietf.org/doc/html/rfc7230#section-3.3.2
*/
    if( md.content_length.count > 0 &&
        md.transfer_encoding.count > 0)
    {
        md.payload = payload::error;
        md.payload_size = 0;
        return;
    }

    if(kind == detail::kind::response)
        goto do_response;

    //--------------------------------------------

/*  The presence of a message body in a
    request is signaled by a Content-Length
    or Transfer-Encoding header field. Request
    message framing is independent of method
    semantics, even if the method does not
    define any use for a message body.
*/
    if(md.content_length.count > 0)
    {
        if(md.content_length.value > 0)
        {
            // non-zero Content-Length
            md.payload = payload::size;
            md.payload_size = md.content_length.value;
            return;
        }
        // Content-Length: 0
        md.payload = payload::none;
        md.payload_size = 0;
        return;
    }
    if(md.transfer_encoding.is_chunked)
    {
        // chunked
        md.payload = payload::chunked;
        md.payload_size = 0;
        return;
    }
    // no payload
    md.payload = payload::none;
    md.payload_size = 0;
    return;

    //--------------------------------------------
do_response:

    if( res.status_int /  100 == 1 ||   // 1xx e.g. Continue
        res.status_int == 204 ||        // No Content
        res.status_int == 304)          // Not Modified
    {
    /*  The correctness of any Content-Length
        here is defined by the particular
        resource, and cannot be determined
        here. In any case there is no payload.
    */
        md.payload = payload::none;
        md.payload_size = 0;
        return;
    }
    if(md.content_length.count > 0)
    {
        if(md.content_length.value > 0)
        {
            // Content-Length > 0
            md.payload = payload::size;
            md.payload_size = md.content_length.value;
            return;
        }
        // Content-Length: 0
        md.payload = payload::none;
        md.payload_size = 0;
        return;
    }
    if(md.transfer_encoding.is_chunked)
    {
        // chunked
        md.payload = payload::chunked;
        md.payload_size = 0;
        return;
    }

    // eof needed
    md.payload = payload::to_eof;
    md.payload_size = 0;
}

//------------------------------------------------

std::size_t
header::
count_crlf(
    core::string_view s) noexcept
{
    auto it = s.data();
    auto len = s.size();
    std::size_t n = 0;
    while(len >= 2)
    {
        if( it[0] == '\r' &&
            it[1] != '\r')
        {
            if(it[1] == '\n')
                n++;
            it += 2;
            len -= 2;
        }
        else
        {
            it++;
            len--;
        }
    }
    return n;
}

static
void
parse_start_line(
    header& h,
    header_limits const& lim,
    std::size_t new_size,
    system::error_code& ec) noexcept
{
    BOOST_ASSERT(h.size == 0);
    BOOST_ASSERT(h.prefix == 0);
    BOOST_ASSERT(h.cbuf != nullptr);
    BOOST_ASSERT(
        h.kind != detail::kind::fields);

    auto const it0 = h.cbuf;
    auto const end = it0 + new_size;
    char const* it = it0;
    if( new_size > lim.max_start_line)
        new_size = lim.max_start_line;
    if(h.kind == detail::kind::request)
    {
        auto rv = grammar::parse(
            it, end, request_line_rule);
        if(! rv)
        {
            ec = rv.error();
            if( ec == grammar::error::need_more &&
                new_size == lim.max_start_line)
                ec = BOOST_HTTP_ERR(
                    error::start_line_limit);
            return;
        }
        // method
        auto sm = std::get<0>(*rv);
        h.req.method = string_to_method(sm);
        h.req.method_len =
            static_cast<header::offset_type>(sm.size());
        // target
        auto st = std::get<1>(*rv);
        h.req.target_len =
            static_cast<header::offset_type>(st.size());
        // version
        switch(std::get<2>(*rv))
        {
        case 10:
            h.version =
                http::version::http_1_0;
            break;
        case 11:
            h.version =
                http::version::http_1_1;
            break;
        default:
        {
            ec = BOOST_HTTP_ERR(
                error::bad_version);
            return;
        }
        }
    }
    else
    {
        auto rv = grammar::parse(
            it, end, status_line_rule);
        if(! rv)
        {
            ec = rv.error();
            if( ec == grammar::error::need_more &&
                new_size == lim.max_start_line)
                ec = BOOST_HTTP_ERR(
                    error::start_line_limit);
            return;
        }
        // version
        switch(std::get<0>(*rv))
        {
        case 10:
            h.version =
                http::version::http_1_0;
            break;
        case 11:
            h.version =
                http::version::http_1_1;
            break;
        default:
        {
            ec = BOOST_HTTP_ERR(
                error::bad_version);
            return;
        }
        }
        // status-code
        h.res.status_int =
            static_cast<unsigned short>(
                std::get<1>(*rv).v);
        h.res.status = std::get<1>(*rv).st;
    }
    h.prefix = static_cast<header::offset_type>(it - it0);
    h.size = h.prefix;
    h.on_start_line();
}

// returns: true if we added a field
static
void
parse_field(
    header& h,
    header_limits const& lim,
    std::size_t new_size,
    system::error_code& ec) noexcept
{
    if( new_size > lim.max_field)
        new_size = lim.max_field;
    auto const it0 = h.cbuf + h.size;
    auto const end = h.cbuf + new_size;
    char const* it = it0;
    auto rv = grammar::parse(
        it, end, field_rule);
    if(rv.has_error())
    {
        ec = rv.error();
        if(ec == grammar::error::end_of_range)
        {
            // final CRLF
            h.size = static_cast<
                header::offset_type>(it - h.cbuf);
            return;
        }
        if( ec == grammar::error::need_more &&
            new_size == lim.max_field)
        {
            ec = BOOST_HTTP_ERR(
                error::field_size_limit);
        }
        return;
    }
    if(h.count >= lim.max_fields)
    {
        ec = BOOST_HTTP_ERR(
            error::fields_limit);
        return;
    }
    if(rv->has_obs_fold)
    {
        // obs fold not allowed in test views
        BOOST_ASSERT(h.buf != nullptr);
        remove_obs_fold(h.buf + h.size, it);
    }
    auto id = string_to_field(rv->name)
        .value_or(header::unknown_field);
    h.size = static_cast<header::offset_type>(it - h.cbuf);

    // add field table entry
    if(h.buf != nullptr)
    {
        auto& e = header::table(
            h.buf + h.cap)[h.count];
        auto const base =
            h.buf + h.prefix;
        e.np = static_cast<header::offset_type>(
            rv->name.data() - base);
        e.nn = static_cast<header::offset_type>(
            rv->name.size());
        e.vp = static_cast<header::offset_type>(
            rv->value.data() - base);
        e.vn = static_cast<header::offset_type>(
            rv->value.size());
        e.id = id;
    }
    ++h.count;
    h.on_insert(id, rv->value);
    ec = {};
}

void
header::
parse(
    std::size_t new_size,
    header_limits const& lim,
    system::error_code& ec) noexcept
{
    if( new_size > lim.max_size)
        new_size = lim.max_size;
    if( this->prefix == 0 &&
        this->kind !=
            detail::kind::fields)
    {
        parse_start_line(
            *this, lim, new_size, ec);
        if(ec)
        {
            if( ec == grammar::error::need_more &&
                new_size == lim.max_fields)
            {
                ec = BOOST_HTTP_ERR(
                    error::headers_limit);
            }
            return;
        }
    }
    for(;;)
    {
        parse_field(
            *this, lim, new_size, ec);
        if(ec)
        {
            if( ec == grammar::error::need_more &&
                new_size == lim.max_size)
            {
                ec = BOOST_HTTP_ERR(
                    error::headers_limit);
                return;
            }
            break;
        }
    }
    if(ec == grammar::error::end_of_range)
        ec = {};
}

} // detail
} // http
} // boost
