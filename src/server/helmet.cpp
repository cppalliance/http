//
// Copyright (c) 2025 Amlal El Mahrouss (amlal at nekernel dot org)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/server/helmet.hpp>
#include <boost/core/exchange.hpp>
#include <algorithm>
#include <vector>

namespace boost {
namespace http_proto {

namespace detail {

    auto setHeaderValues(const std::vector<std::string>& fields_value) -> std::string 
    {
        if (fields_value.empty())
            return {};

        std::string cached_fields;

        for (const auto& field : fields_value)
        {
            cached_fields += field;
            cached_fields += "; ";
        }

        return cached_fields;
    }
    
}

helmet_options::helmet_options()
{
    // Default headers as per helmet.js defaults
    this->set(x_download_options(helmet_download_type::noopen));
    this->set(x_frame_origin(helmet_origin_type::deny));
    this->set(x_xss_protection());
    this->set(x_content_type_options());
    this->set(strict_transport_security(hsts::default_age, hsts::include_subdomains, hsts::preload));
    this->set(cross_origin_opener_policy());
    this->set(cross_origin_resource_policy());
    this->set(origin_agent_cluster());
    this->set(referrer_policy());
    this->set(dns_prefetch_control());
    this->set(permitted_cross_domain_policies());
    this->set(hide_powered_by());
}

helmet_options::~helmet_options() = default;

helmet_options& helmet_options::set(const pair_type& helmet_hdr)
{
    auto it_hdr = std::find_if(headers.begin(), headers.end(), [&helmet_hdr](const pair_type& pair) {
        return pair.first == helmet_hdr.first;
    });
    
    if (it_hdr != headers.end())
    {
        *it_hdr = helmet_hdr;

        return *this;
    }

    headers.emplace_back(helmet_hdr);
    return *this;
}

/**
    @brief private implementation details for the `helmet` middleware.
*/
struct helmet::impl
{
    using pair_type = std::pair<std::string, std::string>;
    using vector_type = std::vector<pair_type>;

    helmet_options options_;
    vector_type cached_headers_{};
};

helmet::
helmet(
    helmet_options helmet_options)
    : impl_(new impl())
{
    impl_->options_ = std::move(helmet_options);

    std::for_each(impl_->options_.headers.begin(), impl_->options_.headers.end(), 
        [this] (const helmet_options::pair_type& hdr) 
    {
        this->impl_->cached_headers_.emplace_back(hdr.first, 
                                                detail::setHeaderValues(hdr.second));
    });
}

helmet::~helmet()
{
    if (impl_ != nullptr)
      delete impl_;
    
    impl_ = nullptr;
}

helmet& helmet::operator=(helmet&& other) noexcept
{
    delete impl_;
    impl_ = boost::exchange(other.impl_, nullptr);

    return *this;
}

helmet::
helmet(helmet&& other) noexcept
{
    delete impl_;
    impl_ = boost::exchange(other.impl_, nullptr);
}

route_result
helmet::
operator()(
    route_params& p) const
{
    std::for_each(impl_->cached_headers_.begin(), impl_->cached_headers_.end(),
        [&p] (const impl::pair_type& hdr)
    {
        // If value is empty, it means we should remove the header (e.g., X-Powered-By)
        if (hdr.second.empty())
        {
            p.res.erase(hdr.first);
        }
        else
        {
            p.res.set(hdr.first, hdr.second);
        }
    });

    return route::next;
}

helmet::csp_policy& helmet::csp_policy::allow(const core::string_view& allow, 
                            const urls::url_view& source)
{
    if (allow.empty())
        detail::throw_invalid_argument();

    static std::vector<core::string_view> const good_schemes{
      "http",
      "https",
      "wss",
      "ws",
      "file",
    };

    bool found_scheme{};

    for (const auto& scheme : good_schemes)
    {
      found_scheme = source.scheme() == scheme;
    }
    if (!found_scheme)
	detail::throw_invalid_argument();
	
    std::string value = allow;

    value += " ";
    value += source.data();

	auto it = std::find_if(list.begin(), list.end(), [&allow](const std::string& in) {
        std::string allow_with_space = std::string(allow) + " ";
        return in.find(allow_with_space) != std::string::npos;
    });
	
    if (it == list.end())
    {
        list.emplace_back(value);
    }
    else
    {
        auto& it_elem = *it;
        
        it_elem += " ";
        it_elem += source.data();
    }

    return *this;
}

helmet::csp_policy& helmet::csp_policy::allow(const core::string_view& allow, const csp_type& type)
{
    if (allow.empty())
        detail::throw_invalid_argument();

    std::string value;

    switch (type)
    {
        case csp_type::sandbox:
        {
            value += " 'sandbox'";
            break;
        }
        case csp_type::none:
        {
            value += " 'none'";
            break;
        }
        case csp_type::unsafe_inline:
        {
            value += " 'unsafe-inline'";
            break;
        }
        case csp_type::unsafe_eval:
        {
            value += " 'unsafe-eval'";
            break;
        }
        case csp_type::strict_dynamic:
        {
            value += " 'strict-dynamic'";
            break;
        }
        case csp_type::report_sample:
        {
            value += " 'report-sample'";
            break;
        }
        case csp_type::self:
        {
            value += " 'self'";
            break;
        }
        default:
            detail::throw_invalid_argument();
            return *this;
    }

	auto it = std::find_if(list.begin(), list.end(), [&allow](const std::string& in) {
        std::string allow_with_space = std::string(allow) + " ";
        return in.find(allow_with_space) != std::string::npos;
    });
	
    if (it == list.end())
    {
        std::string final_result = allow;
        final_result += value;

        list.emplace_back(final_result);
    }
    else
    {
        auto& it_elem = *it;
        it_elem.push_back(' ');
        it_elem += value;
    }

    return *this;
}

helmet::csp_policy::csp_policy() = default;
helmet::csp_policy::~csp_policy() = default;

helmet::csp_policy& helmet::csp_policy::remove(const core::string_view& allow)
{
    if (auto it = std::find_if(list.cbegin(), list.cend(), [&allow](const std::string& in) {
        std::string allow_with_space = std::string(allow) + " ";
        return in.find(allow_with_space) != std::string::npos;
    }); it != list.cend())
    {
        list.erase(it);
    }
    else
    {
        detail::throw_invalid_argument();
    }

    return *this;
}

option_pair x_xss_protection()
{
    return {"X-XSS-Protection", {"0"}};
}

option_pair x_content_type_options()
{
    return {"X-Content-Type-Options", {"nosniff"}};
}

option_pair content_security_policy(const helmet::csp_policy& sp)
{
    if (sp.list.empty())
        detail::throw_invalid_argument();

    return {"Content-Security-Policy", sp.list};
}

option_pair x_frame_origin(const helmet_origin_type& origin)
{
    if (origin == helmet_origin_type::sameorigin)
    {
        return {"X-Frame-Options", {"SAMEORIGIN"}};
    }
    else if (origin == helmet_origin_type::deny)
    {
        return {"X-Frame-Options", {"DENY"}};
    }

    return {};
}

option_pair x_download_options(const helmet_download_type& type)
{
    if (type == helmet_download_type::noopen)
    {
        return {"X-Download-Options", {"noopen"}};
    }

    return {};
}

option_pair cross_origin_opener_policy(const coop_policy_type& policy)
{
    std::string value;

    switch (policy)
    {
        case coop_policy_type::unsafe_none:
            value = "unsafe-none";
            break;
        case coop_policy_type::same_origin_allow_popups:
            value = "same-origin-allow-popups";
            break;
        case coop_policy_type::same_origin:
            value = "same-origin";
            break;
        default:
            return {};
    }

    return {"Cross-Origin-Opener-Policy", {value}};
}

option_pair cross_origin_resource_policy(const corp_policy_type& policy)
{
    std::string value;

    switch (policy)
    {
        case corp_policy_type::same_origin:
            value = "same-origin";
            break;
        case corp_policy_type::same_site:
            value = "same-site";
            break;
        case corp_policy_type::cross_origin:
            value = "cross-origin";
            break;
        default:
            return {};
    }

    return {"Cross-Origin-Resource-Policy", {value}};
}

option_pair cross_origin_embedder_policy(const coep_policy_type& policy)
{
    std::string value;

    switch (policy)
    {
        case coep_policy_type::unsafe_none:
            value = "unsafe-none";
            break;
        case coep_policy_type::require_corp:
            value = "require-corp";
            break;
        case coep_policy_type::credentialless:
            value = "credentialless";
            break;
        default:
            return {};
    }

    return {"Cross-Origin-Embedder-Policy", {value}};
}

option_pair strict_transport_security(std::size_t age, bool include_domains, bool preload)
{
    std::string value = "max-age=" + std::to_string(age);

    if (include_domains)
    {
        value += "; includeSubDomains";
    }

    if (preload)
    {
        value += "; preload";
    }

    return {"Strict-Transport-Security", {value}};
}

option_pair referrer_policy(const referrer_policy_type& policy)
{
    std::string value;

    switch (policy)
    {
        case referrer_policy_type::no_referrer:
            value = "no-referrer";
            break;
        case referrer_policy_type::no_referrer_when_downgrade:
            value = "no-referrer-when-downgrade";
            break;
        case referrer_policy_type::same_origin:
            value = "same-origin";
            break;
        case referrer_policy_type::origin:
            value = "origin";
            break;
        case referrer_policy_type::strict_origin:
            value = "strict-origin";
            break;
        case referrer_policy_type::origin_when_cross_origin:
            value = "origin-when-cross-origin";
            break;
        case referrer_policy_type::strict_origin_when_cross_origin:
            value = "strict-origin-when-cross-origin";
            break;
        case referrer_policy_type::unsafe_url:
            value = "unsafe-url";
            break;
        default:
            return {};
    }

    return {"Referrer-Policy", {value}};
}

option_pair origin_agent_cluster()
{
    return {"Origin-Agent-Cluster", {"?1"}};
}

option_pair dns_prefetch_control(bool allow)
{
    return {"X-DNS-Prefetch-Control", {allow ? "on" : "off"}};
}

option_pair permitted_cross_domain_policies(const cross_domain_policy_type& policy)
{
    std::string value;

    switch (policy)
    {
        case cross_domain_policy_type::none:
            value = "none";
            break;
        case cross_domain_policy_type::master_only:
            value = "master-only";
            break;
        case cross_domain_policy_type::by_content_type:
            value = "by-content-type";
            break;
        case cross_domain_policy_type::by_ftp_filename:
            value = "by-ftp-filename";
            break;
        case cross_domain_policy_type::all:
            value = "all";
            break;
        default:
            return {};
    }

    return {"X-Permitted-Cross-Domain-Policies", {value}};
}

option_pair hide_powered_by()
{
    return {"X-Powered-By", {}};
}
}

}
