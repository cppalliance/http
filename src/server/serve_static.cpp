//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#include <boost/http/server/serve_static.hpp>
#include <boost/http/server/send_file.hpp>
#include <boost/http/field.hpp>
#include <boost/http/file.hpp>
#include <boost/http/status.hpp>
#include <filesystem>
#include <string>

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

#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
#else
    char constexpr path_separator = '/';
#endif
    if(! result.empty() && result.back() == path_separator)
        result.resize(result.size() - 1);

#ifdef BOOST_MSVC
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

// Check if path segment is a dotfile
bool
is_dotfile(core::string_view path) noexcept
{
    auto pos = path.rfind('/');
    if(pos == core::string_view::npos)
        pos = 0;
    else
        ++pos;

    if(pos < path.size() && path[pos] == '.')
        return true;

    return false;
}

} // (anon)

struct serve_static::impl
{
    std::string root;
    serve_static_options opts;

    impl(
        core::string_view root_,
        serve_static_options const& opts_)
        : root(root_)
        , opts(opts_)
    {
    }
};

serve_static::
~serve_static()
{
    delete impl_;
}

serve_static::
serve_static(core::string_view root)
    : serve_static(root, serve_static_options{})
{
}

serve_static::
serve_static(
    core::string_view root,
    serve_static_options const& opts)
    : impl_(new impl(root, opts))
{
}

serve_static::
serve_static(serve_static&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = nullptr;
}

route_task
serve_static::
operator()(route_params& rp) const
{
    // Only handle GET and HEAD
    if(rp.req.method() != method::get &&
        rp.req.method() != method::head)
    {
        if(impl_->opts.fallthrough)
            co_return route::next;

        rp.res.set_status(status::method_not_allowed);
        rp.res.set(field::allow, "GET, HEAD");
        co_return co_await rp.send("");
    }

    // Get the request path
    auto req_path = rp.url.path();

    // Check for dotfiles
    if(is_dotfile(req_path))
    {
        switch(impl_->opts.dotfiles)
        {
        case dotfiles_policy::deny:
            rp.res.set_status(status::forbidden);
            co_return co_await rp.send("Forbidden");

        case dotfiles_policy::ignore:
            if(impl_->opts.fallthrough)
                co_return route::next;
            rp.res.set_status(status::not_found);
            co_return co_await rp.send("Not Found");

        case dotfiles_policy::allow:
            break;
        }
    }

    // Build the file path
    std::string path;
    path_cat(path, impl_->root, req_path);

    // Check if it's a directory
    std::error_code fec;
    bool is_dir = std::filesystem::is_directory(path, fec);
    if(is_dir && ! fec)
    {
        // Check for trailing slash
        if(req_path.empty() || req_path.back() != '/')
        {
            if(impl_->opts.redirect)
            {
                // Redirect to add trailing slash
                std::string location(req_path);
                location += '/';
                rp.res.set_status(status::moved_permanently);
                rp.res.set(field::location, location);
                co_return co_await rp.send("");
            }
        }

        // Try index file
        if(impl_->opts.index)
        {
#ifdef BOOST_MSVC
            path += "\\index.html";
#else
            path += "/index.html";
#endif
        }
    }

    // Prepare file response using send_file utilities
    send_file_options opts;
    opts.etag = impl_->opts.etag;
    opts.last_modified = impl_->opts.last_modified;
    opts.max_age = impl_->opts.max_age;

    send_file_info info;
    send_file_init(info, rp, path, opts);

    // Handle result
    switch(info.result)
    {
    case send_file_result::not_found:
        if(impl_->opts.fallthrough)
            co_return route::next;
        rp.res.set_status(status::not_found);
        co_return co_await rp.send("Not Found");

    case send_file_result::not_modified:
        rp.res.set_status(status::not_modified);
        co_return co_await rp.send("");

    case send_file_result::error:
        // Range error - headers already set by send_file_init
        co_return co_await rp.send("");

    case send_file_result::ok:
        break;
    }

    // Set Accept-Ranges if enabled
    if(impl_->opts.accept_ranges)
        rp.res.set(field::accept_ranges, "bytes");

    // Set Cache-Control with immutable if configured
    if(impl_->opts.immutable && opts.max_age > 0)
    {
        std::string cc = "public, max-age=" +
            std::to_string(opts.max_age) + ", immutable";
        rp.res.set(field::cache_control, cc);
    }

    // For HEAD requests, don't send body
    if(rp.req.method() == method::head)
        co_return co_await rp.send("");

    // Open and stream the file
    file f;
    system::error_code ec;
    f.open(path.c_str(), file_mode::scan, ec);
    if(ec)
    {
        if(impl_->opts.fallthrough)
            co_return route::next;
        rp.res.set_status(status::internal_server_error);
        co_return co_await rp.send("Internal Server Error");
    }

    // Seek to range start if needed
    if(info.is_range && info.range_start > 0)
    {
        f.seek(static_cast<std::uint64_t>(info.range_start), ec);
        if(ec)
        {
            rp.res.set_status(status::internal_server_error);
            co_return co_await rp.send("Internal Server Error");
        }
    }

    // Calculate how much to send
    std::int64_t remaining = info.range_end - info.range_start + 1;

    // Stream file content
    constexpr std::size_t buf_size = 16384;
    char buffer[buf_size];

    while(remaining > 0)
    {
        auto const to_read = static_cast<std::size_t>(
            (std::min)(remaining, static_cast<std::int64_t>(buf_size)));

        auto const n = f.read(buffer, to_read, ec);
        if(ec || n == 0)
            break;

        co_await rp.write(capy::const_buffer(buffer, n));
        remaining -= static_cast<std::int64_t>(n);
    }

    co_return co_await rp.end();
}

} // http
} // boost
