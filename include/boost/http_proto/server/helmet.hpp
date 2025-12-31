//
// Copyright (c) 2025 Amlal El Mahrouss (amlal at nekernel dot org)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#ifndef BOOST_HTTP_PROTO_SERVER_HELMET_HPP
#define BOOST_HTTP_PROTO_SERVER_HELMET_HPP

#include <boost/http_proto/detail/config.hpp>
#include <boost/http_proto/server/route_handler.hpp>

namespace boost {
namespace http_proto {

/** Configuration options for helmet security middleware.

    This structure holds a collection of HTTP security headers
    that will be applied to responses. Headers are stored as
    key-value pairs where each key can have multiple values.

    @see helmet
*/
struct helmet_options
{
    using pair_type = std::pair<std::string, std::vector<std::string>>;
    using map_type  = std::vector<pair_type>;

    /** Collection of security headers to apply */
    map_type headers;

    /** Set or update a security header.

        If a header with the same name already exists, it will be replaced.
        Otherwise, the new header is appended to the collection.	
        @param helmet_hdr The header name and values to set.

        @return A reference to this object for chaining.
    */
    helmet_options& set(const pair_type& helmet_hdr);

    helmet_options();
    ~helmet_options();
};
  
/** Download behavior for X-Download-Options header.

    Controls how browsers handle file downloads.
*/
enum class helmet_download_type
{
    /** Prevent opening files directly in IE8+ */
    noopen,

    /** Disable the X-Download-Options header */
    disabled
};

/** Frame origin policy for X-Frame-Options header.

    Controls whether the page can be embedded in frames.
*/
enum class helmet_origin_type
{
    /** Prevent all framing */
    deny,

    /** Allow framing from same origin only */
    sameorigin
};

/** Content Security Policy directive values.

    Defines standard CSP source expressions.

    @see helmet::csp_policy
*/
enum class csp_type
{
    /** Refers to the current origin */
    self,

    /** Blocks all sources */
    none,

    /** Allows inline scripts/styles */
    unsafe_inline,

    /** Allows eval() and similar */
    unsafe_eval,

    /** Enables strict dynamic mode */
    strict_dynamic,

    /** Enables sandbox restrictions */
    sandbox,

    /** Includes samples in violation reports */
    report_sample
};

/** Cross-Origin-Opener-Policy values.

    Controls cross-origin window isolation.
*/
enum class coop_policy_type
{
    /** No isolation (default browser behavior) */
    unsafe_none,

    /** Allow popups from same origin */
    same_origin_allow_popups,

    /** Strict same-origin isolation */
    same_origin
};

/** Cross-Origin-Resource-Policy values.

    Controls cross-origin resource sharing.
*/
enum class corp_policy_type
{
    /** Only same-origin requests allowed */
    same_origin,

    /** Same-site requests allowed */
    same_site,

    /** Cross-origin requests allowed */
    cross_origin
};

/** Cross-Origin-Embedder-Policy values.

    Controls embedding of cross-origin resources.
*/
enum class coep_policy_type
{
    /** No restrictions (default) */
    unsafe_none,

    /** Require CORP header on cross-origin resources */
    require_corp,

    /** Load cross-origin resources without credentials */
    credentialless
};

/** Referrer-Policy values.

    Controls how much referrer information is sent with requests.
*/
enum class referrer_policy_type
{
    /** Never send referrer */
    no_referrer,

    /** Send referrer for HTTPS→HTTPS, not HTTPS→HTTP */
    no_referrer_when_downgrade,

    /** Send referrer for same-origin requests only */
    same_origin,

    /** Send origin only */
    origin,

    /** Send origin only for HTTPS→HTTPS */
    strict_origin,

    /** Send full URL for same-origin, origin for cross-origin */
    origin_when_cross_origin,

    /** Send origin only for HTTPS→HTTPS, nothing for HTTPS→HTTP */
    strict_origin_when_cross_origin,

    /** Always send full URL (unsafe) */
    unsafe_url
};

/** X-Permitted-Cross-Domain-Policies values.

    Controls cross-domain policy files (Flash, Acrobat).
*/
enum class cross_domain_policy_type
{
    /** No policy files allowed */
    none,

    /** Only master policy file */
    master_only,

    /** Policy files with matching content-type */
    by_content_type,

    /** Policy files via FTP filename */
    by_ftp_filename,

    /** All policy files allowed */
    all
};

/** HSTS constants namespace. */
namespace hsts {
    inline static constexpr bool preload = true;
    inline static constexpr bool no_preload = false;
    inline static constexpr bool include_subdomains = true;
    inline static constexpr bool no_subdomains = false;
    inline static constexpr size_t default_age = 31536000;
}

/** List of allowed sources for CSP directives */
using csp_allow_list = std::vector<std::string>;

/** Header name and values pair */
using option_pair = std::pair<std::string, std::vector<std::string>>;

/** Security middleware inspired by helmet.js.

    Helmet helps secure HTTP responses by setting various
    HTTP security headers. It provides protection against
    common web vulnerabilities like XSS, clickjacking,
    and other attacks.

    @par Example
    @code
    helmet_options opts;
    opts.set(x_frame_origin(helmet_origin_type::deny));

    helmet::csp_policy csp;
    csp.allow("script-src", csp_type::self);
    opts.set(content_security_policy(csp));

    srv.wwwroot.use(helmet(opts));
    @endcode

    @see helmet_options
*/
class helmet
{
    struct impl;
    impl* impl_{};

public:
    BOOST_HTTP_PROTO_DECL
    explicit helmet(
        helmet_options options = {});

    BOOST_HTTP_PROTO_DECL
    ~helmet();

    helmet& operator=(helmet&&) noexcept;
    helmet(helmet&&) noexcept;

    BOOST_HTTP_PROTO_DECL
    route_result
    operator()(route_params& p) const;

    /** Content Security Policy builder.

        Provides a fluent interface for constructing CSP headers
        with multiple directives and sources.

        @par Example
        @code
        helmet::csp_policy policy;
        policy.allow("script-src", csp_type::self)
              .allow("style-src", "https://example.com")
              .allow("img-src", csp_type::none);
        @endcode
    */
    struct csp_policy
    {
        /** List of CSP directives */
        csp_allow_list list;

        BOOST_HTTP_PROTO_DECL
        csp_policy();

        BOOST_HTTP_PROTO_DECL
        ~csp_policy();

        /** Allows a CSP directive with a predefined type.

            @param allow The directive name (e.g., "script-src").
            @param type The CSP source expression type.

            @return A reference to this object for chaining.
        */
        BOOST_HTTP_PROTO_DECL
        csp_policy& allow(const core::string_view& allow,
                                const csp_type& type);

        /** Remove a CSP directive.

            @param allow The directive name to remove.

            @return A reference to this object for chaining.

            @throws std::invalid_argument if the directive is not found.
        */
        BOOST_HTTP_PROTO_DECL
        csp_policy& remove(const core::string_view& allow);

        /** Append a CSP directive with a URL source.

            @param allow The directive name (e.g., "script-src").
            @param source The URL to allow.

            @return A reference to this object for chaining.

            @throws std::invalid_argument if parameters are empty.
        */
        BOOST_HTTP_PROTO_DECL
        csp_policy& allow(const core::string_view& allow,
                                const urls::url_view& source);
    };
};

option_pair x_download_options(const helmet_download_type& type);

option_pair x_frame_origin(const helmet_origin_type& origin);

option_pair x_xss_protection();

option_pair x_content_type_options();

option_pair content_security_policy(const helmet::csp_policy& sp);

/** Return HSTS configuration for the host.
    @param age Maximum age in seconds for HSTS enforcement.
    @param include_subdomains either hsts::include_subdomains or hsts::no_subdomains
    @param preload either hsts::preload or hsts::no_preload
    @note Use constants from the hsts namespace.
    @return Header name and value pair.
*/
option_pair strict_transport_security(
        std::size_t age, 
        bool include_subdomains = hsts::include_subdomains, 
        bool preload = hsts::no_preload);

option_pair cross_origin_opener_policy(const coop_policy_type& policy = coop_policy_type::same_origin);

option_pair cross_origin_resource_policy(const corp_policy_type& policy = corp_policy_type::same_origin);

option_pair cross_origin_embedder_policy(const coep_policy_type& policy = coep_policy_type::require_corp);

option_pair referrer_policy(const referrer_policy_type& policy = referrer_policy_type::no_referrer);

option_pair origin_agent_cluster();

option_pair dns_prefetch_control(bool allow = false);

option_pair permitted_cross_domain_policies(const cross_domain_policy_type& policy = cross_domain_policy_type::none);

option_pair hide_powered_by();

}

}

#endif
