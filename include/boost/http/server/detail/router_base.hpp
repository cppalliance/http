//
// Copyright (c) 2025 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_SERVER_DETAIL_ROUTER_BASE_HPP
#define BOOST_HTTP_SERVER_DETAIL_ROUTER_BASE_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/server/router_types.hpp>
#include <boost/http/method.hpp>
#include <boost/url/url_view.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/capy/io_task.hpp>
#include <boost/assert.hpp>
#include <exception>
#include <string_view>
#include <type_traits>

namespace boost {
namespace http {

template<class>
class basic_router;
class flat_router;

namespace detail {

// implementation for all routers
class BOOST_HTTP_DECL
    router_base
{
    struct impl;
    impl* impl_;

    friend class http::flat_router;

protected:
    using opt_flags = unsigned int;

    enum
    {
        is_invalid = 0,
        is_plain = 1,
        is_error = 2,
        is_router = 4,
        is_exception = 8
    };

    struct BOOST_HTTP_DECL
        handler
    {
        char const kind;
        explicit handler(char kind_) noexcept : kind(kind_) {}
        virtual ~handler() = default;
        virtual auto invoke(route_params_base&) const ->
            route_task = 0;

        // Returns the nested router if this handler wraps one, nullptr otherwise.
        // Used by flat_router::flatten() to recurse into nested routers.
        virtual router_base* get_router() noexcept { return nullptr; }
    };

    using handler_ptr = std::unique_ptr<handler>;

    struct handlers
    {
        std::size_t n;
        handler_ptr* p;
    };

protected:
    using match_result = route_params_base::match_result;
    struct matcher;
    struct entry;
    struct layer;

    ~router_base();
    router_base(opt_flags);
    router_base(router_base&&) noexcept;
    router_base& operator=(router_base&&) noexcept;
    layer& new_layer(std::string_view pattern);
    std::size_t new_layer_idx(std::string_view pattern);
    layer& get_layer(std::size_t idx);
    void add_impl(std::string_view, handlers);
    void add_impl(layer&, http::method, handlers);
    void add_impl(layer&, std::string_view, handlers);
    void set_nested_depth(std::size_t parent_depth);

public:
    /** Maximum nesting depth for routers.

        This limit applies to nested routers added via use().
        Exceeding this limit throws std::length_error at insertion time.
    */
    static constexpr std::size_t max_path_depth = 16;
};

template<class H, class... Args>
concept returns_route_task = std::same_as<
    std::invoke_result_t<H, Args...>, route_task>;

} // detail
} // http
} // boost

#endif
