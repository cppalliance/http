//
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/multipart_form_sink.hpp>
#include <boost/http_proto/rfc/content_disposition_rule.hpp>

#include "src/rfc/detail/rules.hpp"

#include <boost/url/grammar/ci_string.hpp>

namespace boost {
namespace http_proto {

namespace {
std::string
unquote(quoted_token_view qtv)
{
    if(qtv.has_escapes())
        return { qtv.begin(), qtv.end() };

    auto rs = std::string{};
    for(auto it = qtv.begin(); it != qtv.end(); it++)
    {
        if(*it == '\\')
            it++;
        rs.push_back(*it);
    }
    return rs;
}
} // namespace

multipart_form_sink::
multipart_form_sink(
    core::string_view boundary)
{
    leftover_.append("\r\n");
    needle_.append("\r\n--");
    needle_.append(boundary);
}

auto
multipart_form_sink::
parts() const noexcept
    -> boost::span<part const>
{
    return parts_;
}

auto
multipart_form_sink::
on_write(
    buffers::const_buffer b,
    bool more) -> results
{
    system::error_code ec;
    core::string_view sv(
        static_cast<char const*>(b.data()), b.size());

    if(!leftover_.empty())
    {
        for(std::size_t i = 0; i < leftover_.size(); ++i)
        {
            core::string_view nd(needle_);
            if(!nd.starts_with({ &leftover_[i], &*leftover_.cend() }))
                continue;
            nd.remove_prefix(leftover_.size() - i);
            if(sv.size() >= nd.size())
            {
                if(!sv.starts_with(nd))
                    continue;
                parse(true, { leftover_.data(), i }, ec);
                if(ec.failed())
                    goto upcall;
                leftover_.clear();
                sv.remove_prefix(nd.size());
                goto loop_sv;
            }
            if(!more)
                break;
            if(!nd.starts_with(sv))
                continue;
            parse(false, { leftover_.data(), i }, ec);
            if(ec.failed())
                goto upcall;
            leftover_.erase(0, i);
            leftover_.append(sv);
            return { ec , sv.size() };
        }
        // leftover_ cannot contain a needle
        parse(false, leftover_, ec);
        if(ec.failed())
            goto upcall;
        leftover_.clear();
    }

loop_sv:
    for(char const* it = sv.begin(); it != sv.end(); ++it)
    {
        if(*it == '\r')
        {
            core::string_view rm(it, sv.end());
            if(rm.size() >= needle_.size())
            {
                if(std::equal(
                    needle_.rbegin(),
                    needle_.rend(),
                    rm.rend() - needle_.size()))
                {
                    parse(true, { sv.begin(), it }, ec);
                    if(ec.failed())
                        goto upcall;
                    sv = { it + needle_.size(), sv.end() };
                    goto loop_sv;
                }
                continue;
            }
            if(!more)
                break;
            if(!core::string_view(needle_).starts_with(rm))
                continue;
            parse(false, { sv.begin(), it }, ec);
            if(ec.failed())
                goto upcall;
            leftover_.append(it, sv.end());
            sv = {};
            goto upcall;
        }
    }
    parse(false, sv, ec);
    if(!ec.failed())
        sv= {};

upcall:
    return { ec, b.size() - sv.size() };
}

void
multipart_form_sink::
parse(
    bool match,
    core::string_view b,
    system::error_code& ec)
{
loop:
    switch(state_)
    {
    case state::preamble:
        if(match)
            state_ = state::post_boundary0;
        break;
    case state::post_boundary0:
        if(b.empty())
            break;
        switch(b[0])
        {
        case '\r':
            state_ = state::post_boundary1;
            break;
        case '-':
            state_ = state::post_boundary2;
            break;
        default:
            ec = http_proto::error::bad_payload;
            return;
        }
        b.remove_prefix(1);
        goto loop;
    case state::post_boundary1:
        if(b.empty())
            break;
        if(b[0] != '\n')
        {
            ec = http_proto::error::bad_payload;
            return;
        }
        b.remove_prefix(1);
        state_ = state::header;
        goto loop;
    case state::post_boundary2:
        if(b.empty())
            break;
        if(b[0] != '-')
        {
            ec = http_proto::error::bad_payload;
            return;
        }
        b.remove_prefix(1);
        state_ = state::finished;
        break;
    case state::header:
    {
        auto const s0 = header_.size();
        header_.append(b);
        auto const pos = header_.find("\r\n\r\n");
        if(pos == std::string::npos)
            break;
        header_.erase(pos + 4);
        b.remove_prefix(header_.size() - s0);

        // parse fields

        auto const fields = grammar::parse(
            header_,
            grammar::range_rule(detail::field_rule));

        if(!fields)
        {
            ec = http_proto::error::bad_payload;
            return;
        }

        part part{};

        for(auto&& field : fields.value())
        {
            if(field.has_obs_fold)
                continue; // TODO

            if(grammar::ci_is_equal(field.name, "Content-Disposition"))
            {
                // parse Content-Disposition
                auto cd = grammar::parse(field.value, content_disposition_rule);
                if(!cd || !grammar::ci_is_equal(cd->type, "form-data"))
                {
                    ec = http_proto::error::bad_payload;
                    return;
                }
                for(auto && param : cd->params)
                {
                    if(grammar::ci_is_equal(param.name, "name"))
                    {
                        part.name = unquote(param.value);
                    }
                    else if(grammar::ci_is_equal(param.name, "filename"))
                    {
                        auto& ff = part.content.emplace<file_field>();
                        ff.name = unquote(param.value);

                        // TODO
                        ff.path = ff.name;
                        file_.open(
                            ff.path.c_str(),
                            capy::file_mode::write_new,
                            ec);
                        if(ec.failed())
                            return;
                    }
                }
            }
            else if(grammar::ci_is_equal(field.name, "Content-Type"))
            {
                part.content_type = field.value;
            }
        }

        if(part.name.empty())
        {
            ec = http_proto::error::bad_payload;
            return;
        }

        header_.clear();
        parts_.push_back(std::move(part));
        state_ = state::content;
        BOOST_FALLTHROUGH;
    }
    case state::content:
        if(auto* p = variant2::get_if<text_field>(
            &parts_.back().content))
        {
            p->data.append(b);
        }
        else
        {
            file_.write(b.data(), b.size(), ec);
            if(ec.failed())
                return;
            if(match)
            {
                file_.close(ec);
                if(ec.failed())
                    return;
            }
        }
        if(match)
            state_ = state::post_boundary0;
        break;
    case state::finished:
        break;
    }
}

} // http_proto
} // boost
