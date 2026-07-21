//
// Copyright (c) 2021 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2025 Mohammad Nejati
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_FIELD_HPP
#define BOOST_HTTP_FIELD_HPP

#include <boost/http/detail/config.hpp>
#include <boost/core/detail/string_view.hpp>
#include <boost/optional.hpp>
#include <iosfwd>

namespace boost {
namespace http {

/** HTTP field name constants.
*/
enum class field : unsigned short
{
    accept = 1,
    accept_ch,
    accept_charset, // Deprecated
    accept_encoding,
    accept_language,
    accept_patch,
    accept_post,
    accept_ranges,
    accept_signature,
    access_control_allow_credentials,
    access_control_allow_headers,
    access_control_allow_methods,
    access_control_allow_origin,
    access_control_expose_headers,
    access_control_max_age,
    access_control_request_headers,
    access_control_request_method,
    age,
    allow,
    alt_svc,
    alt_used,
    authorization,
    cache_control,
    clear_site_data,
    connection,
    content_digest,
    content_disposition,
    content_dpr, // Non-standard // Deprecated
    content_encoding,
    content_language,
    content_length,
    content_location,
    content_range,
    content_security_policy,
    content_security_policy_report_only,
    content_type,
    cookie,
    cross_origin_embedder_policy,
    cross_origin_opener_policy,
    cross_origin_resource_policy,
    date,
    deprecation,
    device_memory,
    digest,
    dnt, // Non-standard // Deprecated
    dpr, // Non-standard // Deprecated
    etag,
    expect,
    expect_ct, // Deprecated
    expires,
    forwarded,
    from,
    host,
    http2_settings, // Deprecated
    if_match,
    if_modified_since,
    if_none_match,
    if_range,
    if_unmodified_since,
    keep_alive,
    last_modified,
    link,
    location,
    max_forwards,
    origin,
    origin_agent_cluster,
    pragma, // Deprecated
    prefer,
    preference_applied,
    priority,
    proxy_authenticate,
    proxy_authorization,
    proxy_connection,
    range,
    referer,
    referrer_policy,
    refresh,
    report_to, // Non-standard // Deprecated
    reporting_endpoints,
    repr_digest,
    retry_after,
    sec_ch_ua_full_version, // Deprecated
    sec_fetch_dest,
    sec_fetch_mode,
    sec_fetch_site,
    sec_fetch_user,
    sec_purpose,
    sec_websocket_accept,
    sec_websocket_extensions,
    sec_websocket_key,
    sec_websocket_protocol,
    sec_websocket_version,
    server,
    server_timing,
    service_worker,
    service_worker_allowed,
    service_worker_navigation_preload,
    set_cookie,
    set_login,
    signature,
    signature_input,
    sourcemap,
    strict_transport_security,
    te,
    timing_allow_origin,
    tk, // Non-standard // Deprecated
    trailer,
    transfer_encoding,
    upgrade,
    upgrade_insecure_requests,
    user_agent,
    vary,
    via,
    viewport_width, // Non-standard // Deprecated
    want_content_digest,
    want_repr_digest,
    warning, // Deprecated
    width, // Non-standard // Deprecated
    www_authenticate,
    x_content_type_options,
    x_dns_prefetch_control, // Non-standard
    x_forwarded_for, // Non-standard
    x_forwarded_host, // Non-standard
    x_forwarded_proto, // Non-standard
    x_frame_options,
    x_permitted_cross_domain_policies, // Non-standard
    x_powered_by, // Non-standard
    x_robots_tag, // Non-standard
    x_xss_protection // Non-standard // Deprecated
};

//------------------------------------------------

/** Return the header name for a field name constant.

    @param f The field name constant to convert.
*/
BOOST_HTTP_DECL
core::string_view
to_string(field f);

/** Return the field name constant for a header name.

    The string comparison is case-insensitive.

    @param s The string representing a header name.
*/
BOOST_HTTP_DECL
boost::optional<field>
string_to_field(
    core::string_view s) noexcept;

/** Write the header name for a field name constant to an output stream.

    @return A reference to the output stream.

    @param os The output stream to write to.

    @param f The field name constant to use.
*/
BOOST_HTTP_DECL
std::ostream&
operator<<(std::ostream& os, field f);

} // http
} // boost

#endif
