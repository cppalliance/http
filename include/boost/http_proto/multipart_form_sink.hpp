//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_MULTIPART_FORM_SINK_HPP
#define BOOST_HTTP_PROTO_MULTIPART_FORM_SINK_HPP

#include <boost/capy/file.hpp>
#include <boost/http_proto/sink.hpp>

#include <boost/optional.hpp>
#include <boost/variant2.hpp>

#include <vector>

namespace boost {
namespace http_proto {

class BOOST_HTTP_PROTO_SYMBOL_VISIBLE multipart_form_sink
    : public http_proto::sink
{
public:
    struct file_field
    {
        std::string name;
        std::string path;
    };

    struct text_field
    {
        std::string data;
    };

    struct part
    {
        std::string name;
        variant2::variant<text_field, file_field> content;
        boost::optional<std::string> content_type;
    };

    BOOST_HTTP_PROTO_DECL
    explicit
    multipart_form_sink(
        core::string_view boundary);

    BOOST_HTTP_PROTO_DECL
    boost::span<part const>
    parts() const noexcept;

private:
    BOOST_HTTP_PROTO_DECL
    results
    on_write(
        buffers::const_buffer b,
        bool more) override;

    void    
    parse(
        bool match,
        core::string_view b,
        system::error_code& ec);

    enum class state
    {
        preamble,
        post_boundary0,
        post_boundary1,
        post_boundary2,
        header,
        content,
        finished
    };

    state state_ = state::preamble;
    std::string needle_;
    std::string leftover_;
    std::string header_;
    capy::file file_;
    std::vector<part> parts_;
};

} // http_proto
} // boost

#endif
