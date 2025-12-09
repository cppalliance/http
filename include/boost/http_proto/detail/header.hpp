//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2024 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_DETAIL_HEADER_HPP
#define BOOST_HTTP_PROTO_DETAIL_HEADER_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/error.hpp>
#include <boost/http_proto/field.hpp>
#include <boost/http_proto/metadata.hpp>
#include <boost/http_proto/method.hpp>
#include <boost/http_proto/status.hpp>
#include <boost/http_proto/version.hpp>

#include <boost/assert.hpp>
#include <boost/core/detail/string_view.hpp>

#include <cstdint>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace boost {
namespace http_proto {

class fields_base;
struct header_limits;

namespace detail {

enum kind : unsigned char
{
    fields = 0,
    request,
    response,
};

struct empty
{
    kind param;
};


struct BOOST_HTTP_PROTO_DECL header
{
    // +------------+---------+------+------------+-----------------------------+
    // | start-line | headers | \r\n | free space | entry[count-1] ... entry[0] |
    // +------------+---------+------+------------+-----------------------------+
    // ^            ^                ^                                          ^
    // buf          buf+prefix       buf+size                                   buf+cap

#ifdef BOOST_HTTP_PROTO_TEST_LIMITS
    using offset_type = std::uint8_t;
#else
    using offset_type = std::uint32_t;
#endif

    static constexpr
    std::size_t max_offset =
        std::numeric_limits<
            offset_type>::max();

    static constexpr
    field unknown_field =
        static_cast<field>(0);

    struct entry
    {
        offset_type np;   // name pos
        offset_type nn;   // name size
        offset_type vp;   // value pos
        offset_type vn;   // value size
        field id;

        entry operator+(
            std::size_t dv) const noexcept;
        entry operator-(
            std::size_t dv) const noexcept;
    };

    struct table
    {
        explicit
        table(
            void* end) noexcept
            : p_(reinterpret_cast<
                entry*>(end))
        {
        }

        entry&
        operator[](
            std::size_t i) const noexcept
        {
            return p_[-1 * (
                static_cast<
                    long>(i) + 1)];
        }

    private:
        entry* p_;
    };

    struct fld_t
    {
    };

    struct req_t
    {
        offset_type method_len;
        offset_type target_len;
        http_proto::method method;
    };

    struct res_t
    {
        unsigned short status_int;
        http_proto::status status;
    };

    //--------------------------------------------

    detail::kind kind;
    char const* cbuf = nullptr;
    char* buf = nullptr;
    std::size_t cap = 0;

    offset_type size = 0;
    offset_type count = 0;
    offset_type prefix = 0;

    http_proto::version version =
        http_proto::version::http_1_1;
    metadata md;

    union
    {
        fld_t fld;
        req_t req;
        res_t res;
    };

private:
    struct fields_tag {};
    struct request_tag {};
    struct response_tag {};

    constexpr header(fields_tag) noexcept;
    constexpr header(request_tag) noexcept;
    constexpr header(response_tag) noexcept;

public:
    // in fields_base.hpp
    static header&
    get(fields_base& f) noexcept;

    static header const*
    get_default(detail::kind k) noexcept;

    // called from parser
    explicit header(empty) noexcept;

    header(detail::kind) noexcept;

    void swap(header&) noexcept;

    bool keep_alive() const noexcept;

    static std::size_t bytes_needed(
        std::size_t size, std::size_t count) noexcept;
    static std::size_t table_space(
        std::size_t count) noexcept;

    std::size_t table_space() const noexcept;

    table tab() const noexcept;
    entry* tab_() const noexcept;
    bool is_default() const noexcept;
    std::size_t find(field) const noexcept;
    std::size_t find(core::string_view) const noexcept;
    void copy_table(void*, std::size_t) const noexcept;
    void copy_table(void*) const noexcept;
    void assign_to(header&) const noexcept;

    // metadata

    std::size_t maybe_count(field) const noexcept;
    bool is_special(field) const noexcept;
    void on_start_line();
    void on_insert(field, core::string_view);
    void on_erase(field);
    void on_insert_connection(core::string_view);
    void on_insert_content_length(core::string_view);
    void on_insert_expect(core::string_view);
    void on_insert_transfer_encoding(core::string_view);
    void on_insert_content_encoding(core::string_view);
    void on_insert_upgrade(core::string_view);
    void on_erase_connection();
    void on_erase_content_length();
    void on_erase_expect();
    void on_erase_transfer_encoding();
    void on_erase_content_encoding();
    void on_erase_upgrade();
    void on_erase_all(field);
    void update_payload() noexcept;

    // parsing

    static std::size_t
    count_crlf(core::string_view s) noexcept;

    void parse(
        std::size_t,
        header_limits const&,
        system::error_code&) noexcept;
};

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif


} // detail
} // http_proto
} // boost

#endif
