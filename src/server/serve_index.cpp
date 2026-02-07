//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/serve_index.hpp>
#include <boost/http/server/accepts.hpp>
#include <boost/http/server/escape_html.hpp>
#include <boost/http/server/encode_url.hpp>
#include <boost/http/field.hpp>
#include <boost/http/status.hpp>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace boost {
namespace http {

namespace {

// Append an HTTP rel-path to a local filesystem path.
void
path_cat(
    std::string& result,
    core::string_view prefix,
    core::string_view suffix)
{
    result = prefix;

#ifdef _WIN32
    char constexpr path_separator = '\\';
#else
    char constexpr path_separator = '/';
#endif
    if(! result.empty() && result.back() == path_separator)
        result.resize(result.size() - 1);

#ifdef _WIN32
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#endif
    for(auto const& c : suffix)
    {
        if(c == '/')
            result.push_back(path_separator);
        else
            result.push_back(c);
    }
}

struct dir_entry
{
    std::string name;
    bool is_dir = false;
    std::uint64_t size = 0;
    std::uint64_t mtime = 0;
};

// Directories first, then case-insensitive alphabetical
bool
entry_less(
    dir_entry const& a,
    dir_entry const& b) noexcept
{
    if(a.is_dir != b.is_dir)
        return a.is_dir;

    // Case-insensitive compare
    auto const& an = a.name;
    auto const& bn = b.name;
    auto const n = (std::min)(an.size(), bn.size());
    for(std::size_t i = 0; i < n; ++i)
    {
        auto ac = static_cast<unsigned char>(an[i]);
        auto bc = static_cast<unsigned char>(bn[i]);
        if(ac >= 'A' && ac <= 'Z') ac += 32;
        if(bc >= 'A' && bc <= 'Z') bc += 32;
        if(ac != bc)
            return ac < bc;
    }
    return an.size() < bn.size();
}

std::uint64_t
to_epoch(std::filesystem::file_time_type tp)
{
    auto const sctp = std::chrono::clock_cast<
        std::chrono::system_clock>(tp);
    auto const dur = sctp.time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<
            std::chrono::seconds>(dur).count());
}

std::string
format_size(std::uint64_t bytes)
{
    if(bytes < 1024)
        return std::to_string(bytes) + " B";
    if(bytes < 1024 * 1024)
        return std::to_string(bytes / 1024) + " KB";
    if(bytes < 1024 * 1024 * 1024)
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    return std::to_string(
        bytes / (1024ULL * 1024 * 1024)) + " GB";
}

std::string
format_time(std::uint64_t epoch)
{
    if(epoch == 0)
        return "-";

    auto const t = static_cast<std::time_t>(epoch);
    std::tm tm;
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf),
        "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}


std::string
render_html(
    core::string_view dir,
    std::vector<dir_entry> const& entries,
    bool show_parent)
{
    std::string body;
    body.reserve(4096);

    body.append(
        "<!DOCTYPE html>\n"
        "<html>\n<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" "
            "content=\"width=device-width\">\n"
        "<title>Index of ");
    body.append(escape_html(dir));
    body.append(
        "</title>\n"
        "<style>\n"
        "body { font-family: -apple-system, "
            "BlinkMacSystemFont, sans-serif; "
            "margin: 2em; }\n"
        "h1 { font-size: 1.4em; }\n"
        "table { border-collapse: collapse; "
            "width: 100%; max-width: 900px; }\n"
        "th, td { text-align: left; "
            "padding: 0.4em 1em; }\n"
        "th { border-bottom: 2px solid #ddd; }\n"
        "td { border-bottom: 1px solid #eee; }\n"
        "a { text-decoration: none; "
            "color: #0366d6; }\n"
        "a:hover { text-decoration: underline; }\n"
        ".size, .date { color: #586069; }\n"
        "</style>\n"
        "</head>\n<body>\n"
        "<h1>Index of ");
    body.append(escape_html(dir));
    body.append("</h1>\n");

    body.append(
        "<table>\n"
        "<tr><th>Name</th>"
        "<th>Size</th>"
        "<th>Modified</th></tr>\n");

    if(show_parent)
    {
        body.append(
            "<tr><td><a href=\"../\">"
            "..</a></td>"
            "<td class=\"size\">-</td>"
            "<td class=\"date\">-</td></tr>\n");
    }

    for(auto const& e : entries)
    {
        auto display_name = escape_html(e.name);
        auto href = encode_url(e.name);
        if(e.is_dir)
            href += '/';

        body.append("<tr><td><a href=\"");
        body.append(href);
        body.append("\">");
        body.append(display_name);
        if(e.is_dir)
            body.append("/");
        body.append("</a></td>");
        body.append("<td class=\"size\">");
        body.append(e.is_dir ? "-" : format_size(e.size));
        body.append("</td>");
        body.append("<td class=\"date\">");
        body.append(format_time(e.mtime));
        body.append("</td></tr>\n");
    }

    body.append("</table>\n</body>\n</html>\n");
    return body;
}

std::string
render_json(
    std::vector<dir_entry> const& entries)
{
    std::string body;
    body.reserve(1024);
    body.push_back('[');

    bool first = true;
    for(auto const& e : entries)
    {
        if(! first)
            body.push_back(',');
        first = false;

        body.append("{\"name\":\"");

        // Escape JSON string
        for(auto c : e.name)
        {
            switch(c)
            {
            case '"':  body.append("\\\""); break;
            case '\\': body.append("\\\\"); break;
            case '\n': body.append("\\n");  break;
            case '\r': body.append("\\r");  break;
            case '\t': body.append("\\t");  break;
            default:   body.push_back(c);   break;
            }
        }

        body.append("\",\"type\":\"");
        body.append(e.is_dir ? "directory" : "file");
        body.append("\",\"size\":");
        body.append(std::to_string(e.size));
        body.append(",\"mtime\":");
        body.append(std::to_string(e.mtime));
        body.push_back('}');
    }

    body.push_back(']');
    return body;
}

std::string
render_plain(
    std::vector<dir_entry> const& entries)
{
    std::string body;
    body.reserve(1024);
    for(auto const& e : entries)
    {
        body.append(e.name);
        if(e.is_dir)
            body.push_back('/');
        body.push_back('\n');
    }
    return body;
}

} // (anon)

//------------------------------------------------

struct serve_index::impl
{
    std::string root;
    serve_index::options opts;

    impl(
        core::string_view root_,
        serve_index::options const& opts_)
        : root(root_)
        , opts(opts_)
    {
    }
};

serve_index::
~serve_index()
{
    delete impl_;
}

serve_index::
serve_index(core::string_view root)
    : serve_index(root, options{})
{
}

serve_index::
serve_index(
    core::string_view root,
    options const& opts)
    : impl_(new impl(root, opts))
{
}

serve_index::
serve_index(serve_index&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = nullptr;
}

route_task
serve_index::
operator()(route_params& rp) const
{
    // Only handle GET and HEAD
    if(rp.req.method() != method::get &&
        rp.req.method() != method::head)
    {
        if(impl_->opts.fallthrough)
            co_return route_next;

        rp.res.set_status(status::method_not_allowed);
        rp.res.set(field::allow, "GET, HEAD, OPTIONS");
        auto [ec] = co_await rp.send();
        if(ec)
            co_return route_error(ec);
        co_return route_done;
    }

    auto req_path = rp.url.path();

    // Build filesystem path
    std::string path;
    path_cat(path, impl_->root, req_path);

    // Must be a directory
    std::error_code fec;
    auto fs_status = std::filesystem::status(path, fec);
    if(fec || fs_status.type() !=
        std::filesystem::file_type::directory)
        co_return route_next;

    // Redirect if missing trailing slash
    if(req_path.empty() || req_path.back() != '/')
    {
        std::string location(req_path);
        location += '/';
        rp.res.set_status(status::moved_permanently);
        rp.res.set(field::location, location);
        auto [ec] = co_await rp.send("");
        if(ec)
            co_return route_error(ec);
        co_return route_done;
    }

    // Read directory entries
    std::vector<dir_entry> entries;
    {
        std::filesystem::directory_iterator it(path, fec);
        if(fec)
            co_return route_next;

        for(auto const& de :
            std::filesystem::directory_iterator(path, fec))
        {
            auto name = de.path().filename().string();

            // Skip hidden files unless configured
            if(! impl_->opts.hidden &&
                ! name.empty() && name[0] == '.')
                continue;

            dir_entry e;
            e.name = std::move(name);

            std::error_code sec;
            e.is_dir = de.is_directory(sec);
            if(! e.is_dir)
                e.size = de.file_size(sec);
            auto lwt = de.last_write_time(sec);
            if(! sec)
                e.mtime = to_epoch(lwt);

            entries.push_back(std::move(e));
        }
    }

    std::sort(entries.begin(), entries.end(), entry_less);

    // Determine ".." display
    std::filesystem::path root_canonical(impl_->root);
    std::filesystem::path dir_canonical(path);
    {
        std::error_code ec2;
        root_canonical =
            std::filesystem::canonical(root_canonical, ec2);
        dir_canonical =
            std::filesystem::canonical(dir_canonical, ec2);
    }
    bool show_up = impl_->opts.show_parent &&
        dir_canonical != root_canonical;

    // Content negotiation
    accepts ac( rp.req );
    auto type = ac.type({ "html", "json", "text" });

    std::string body;
    std::string_view content_type;
    if( type == "json" )
    {
        body = render_json(entries);
        content_type = "application/json; charset=utf-8";
    }
    else if( type == "text" )
    {
        body = render_plain(entries);
        content_type = "text/plain; charset=utf-8";
    }
    else
    {
        body = render_html(req_path, entries, show_up);
        content_type = "text/html; charset=utf-8";
    }

    rp.res.set(field::content_type, content_type);
    rp.res.set("X-Content-Type-Options", "nosniff");

    auto [ec] = co_await rp.send(body);
    if(ec)
        co_return route_error(ec);
    co_return route_done;
}

} // http
} // boost
