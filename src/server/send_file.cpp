//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/send_file.hpp>
#include <boost/http/server/etag.hpp>
#include <boost/http/server/fresh.hpp>
#include <boost/http/server/mime_types.hpp>
#include <boost/http/server/range_parser.hpp>
#include <boost/http/field.hpp>
#include <boost/http/status.hpp>
#include <ctime>
#include <filesystem>

namespace boost {
namespace http {

namespace {

// Get file stats
bool
get_file_stats(
    core::string_view path,
    std::uint64_t& size,
    std::uint64_t& mtime)
{
    system::error_code ec;
    std::filesystem::path p(path.begin(), path.end());

    auto status = std::filesystem::status(p, ec);
    if(ec.failed() || ! std::filesystem::is_regular_file(status))
        return false;

    size = static_cast<std::uint64_t>(
        std::filesystem::file_size(p, ec));
    if(ec.failed())
        return false;

    auto ftime = std::filesystem::last_write_time(p, ec);
    if(ec.failed())
        return false;

    // Convert to Unix timestamp
    auto const sctp = std::chrono::time_point_cast<
        std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() +
            std::chrono::system_clock::now());
    mtime = static_cast<std::uint64_t>(
        std::chrono::system_clock::to_time_t(sctp));

    return true;
}

} // (anon)

std::string
format_http_date(std::uint64_t mtime)
{
    std::time_t t = static_cast<std::time_t>(mtime);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    char buf[64];
    std::strftime(buf, sizeof(buf),
        "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return std::string(buf);
}

void
send_file_init(
    send_file_info& info,
    route_params& rp,
    core::string_view path,
    send_file_options const& opts)
{
    info = send_file_info{};

    // Get file stats
    if(! get_file_stats(path, info.size, info.mtime))
    {
        info.result = send_file_result::not_found;
        return;
    }

    // Determine content type
    if(! opts.content_type.empty())
    {
        info.content_type = opts.content_type;
    }
    else
    {
        auto ct = mime_types::content_type(path);
        if(ct.empty())
            ct = "application/octet-stream";
        info.content_type = std::move(ct);
    }

    // Generate ETag if enabled
    if(opts.etag)
    {
        info.etag = etag(info.size, info.mtime);
        rp.res.set(field::etag, info.etag);
    }

    // Set Last-Modified if enabled
    if(opts.last_modified)
    {
        info.last_modified = format_http_date(info.mtime);
        rp.res.set(field::last_modified, info.last_modified);
    }

    // Set Cache-Control
    if(opts.max_age > 0)
    {
        std::string cc = "public, max-age=" +
            std::to_string(opts.max_age);
        rp.res.set(field::cache_control, cc);
    }

    // Check freshness (conditional GET)
    if(is_fresh(rp.req, rp.res))
    {
        info.result = send_file_result::not_modified;
        return;
    }

    // Set Content-Type
    rp.res.set(field::content_type, info.content_type);

    // Handle Range header
    auto range_header = rp.req.value_or(field::range, "");
    if(! range_header.empty())
    {
        auto range_result = parse_range(
            static_cast<std::int64_t>(info.size),
            range_header);

        if(range_result.type == range_result_type::ok &&
            ! range_result.ranges.empty())
        {
            // Use first range only (simplification)
            auto const& range = range_result.ranges[0];
            info.is_range = true;
            info.range_start = range.start;
            info.range_end = range.end;

            // Set 206 Partial Content
            rp.res.set_status(status::partial_content);

            auto const content_length =
                range.end - range.start + 1;
            rp.res.set_payload_size(
                static_cast<std::uint64_t>(content_length));

            // Content-Range header
            std::string cr = "bytes " +
                std::to_string(range.start) + "-" +
                std::to_string(range.end) + "/" +
                std::to_string(info.size);
            rp.res.set(field::content_range, cr);

            info.result = send_file_result::ok;
            return;
        }

        if(range_result.type == range_result_type::unsatisfiable)
        {
            rp.res.set_status(
                status::range_not_satisfiable);
            rp.res.set(field::content_range,
                "bytes */" + std::to_string(info.size));
            info.result = send_file_result::error;
            return;
        }
        // If malformed, ignore and serve full content
    }

    // Full content response
    rp.res.set_status(status::ok);
    rp.res.set_payload_size(info.size);
    info.range_start = 0;
    info.range_end = static_cast<std::int64_t>(info.size) - 1;
    info.result = send_file_result::ok;
}

} // http
} // boost
