//
// Copyright (c) 2025 Amlal El Mahrouss (amlal at nekernel dot org)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http_proto
//

#include <boost/http_proto/server/helmet.hpp>
#include "test_suite.hpp"

namespace boost {
namespace http_proto {

struct helmet_test
{
    void
    run()
    {
        // Test X-Download-Options
        {
            helmet_options opt;

            opt.set(x_download_options(helmet_download_type::noopen));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("X-Download-Options") > 0);
        }

        // Test X-Frame-Options with DENY
        {
            helmet_options opt;

            opt.set(x_frame_origin(helmet_origin_type::deny));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("X-Frame-Options") > 0);
        }

        // Test X-Frame-Options with SAMEORIGIN
        {
            helmet_options opt;

            opt.set(x_frame_origin(helmet_origin_type::sameorigin));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("X-Frame-Options") > 0);
        }

        // Test X-XSS-Protection
        {
            helmet_options opt;

            opt.set(x_xss_protection());

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("X-XSS-Protection") > 0);
        }

        // Test X-Content-Type-Options
        {
            helmet_options opt;

            opt.set(x_content_type_options());

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("X-Content-Type-Options") > 0);
        }

        // Test Strict-Transport-Security with defaults
        {
            helmet_options opt;

            opt.set(strict_transport_security(hsts::default_age));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Strict-Transport-Security") > 0);
        }

        // Test Strict-Transport-Security with custom options
        {
            helmet_options opt;

            opt.set(strict_transport_security(86400, false, false));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Strict-Transport-Security") > 0);
        }

        // Test Cross-Origin-Opener-Policy
        {
            helmet_options opt;

            opt.set(cross_origin_opener_policy(coop_policy_type::same_origin));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Cross-Origin-Opener-Policy") > 0);
        }

        // Test Cross-Origin-Resource-Policy
        {
            helmet_options opt;

            opt.set(cross_origin_resource_policy(corp_policy_type::same_origin));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Cross-Origin-Resource-Policy") > 0);
        }

        // Test Cross-Origin-Embedder-Policy
        {
            helmet_options opt;

            opt.set(cross_origin_embedder_policy(coep_policy_type::require_corp));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Cross-Origin-Embedder-Policy") > 0);
        }

        // Test Referrer-Policy
        {
            helmet_options opt;

            opt.set(referrer_policy(referrer_policy_type::no_referrer));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Referrer-Policy") > 0);
        }

        // Test Origin-Agent-Cluster
        {
            helmet_options opt;

            opt.set(origin_agent_cluster());

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Origin-Agent-Cluster") > 0);
        }

        // Test X-DNS-Prefetch-Control
        {
            helmet_options opt;

            opt.set(dns_prefetch_control(false));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("X-DNS-Prefetch-Control") > 0);
        }

        // Test X-Permitted-Cross-Domain-Policies
        {
            helmet_options opt;

            opt.set(permitted_cross_domain_policies(cross_domain_policy_type::none));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("X-Permitted-Cross-Domain-Policies") > 0);
        }

        // Test hide_powered_by
        {
            helmet_options opt;

            opt.set(hide_powered_by());

            helmet helmet{opt};
            route_params p;

            // Add X-Powered-By header first
            p.res.set("X-Powered-By", "Express");

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            // After helmet middleware, X-Powered-By should be removed
            BOOST_TEST(p.res.count("X-Powered-By") == 0);
        }

        // Test Content-Security-Policy with CSP builder
        {
            helmet_options opt;
            helmet::csp_policy csp;

            csp.allow("default-src", csp_type::self)
                .allow("script-src", csp_type::self)
                .allow("style-src", csp_type::self);

            opt.set(content_security_policy(csp));

            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            BOOST_TEST(p.res.count("Content-Security-Policy") > 0);
        }

        // Test default helmet configuration
        {
            helmet_options opt;
            helmet helmet{opt};
            route_params p;

            auto ec = helmet(p);

            BOOST_TEST(ec == route::next);
            // Check that default headers are set
            BOOST_TEST(p.res.count("X-Download-Options") > 0);
            BOOST_TEST(p.res.count("X-Frame-Options") > 0);
            BOOST_TEST(p.res.count("X-XSS-Protection") > 0);
            BOOST_TEST(p.res.count("X-Content-Type-Options") > 0);
            BOOST_TEST(p.res.count("Strict-Transport-Security") > 0);
            BOOST_TEST(p.res.count("Cross-Origin-Opener-Policy") > 0);
            BOOST_TEST(p.res.count("Cross-Origin-Resource-Policy") > 0);
            BOOST_TEST(p.res.count("Origin-Agent-Cluster") > 0);
            BOOST_TEST(p.res.count("Referrer-Policy") > 0);
            BOOST_TEST(p.res.count("X-DNS-Prefetch-Control") > 0);
            BOOST_TEST(p.res.count("X-Permitted-Cross-Domain-Policies") > 0);
        }
    }
};

TEST_SUITE(
    helmet_test,
    "boost.http_proto.server.helmet");
}

}
