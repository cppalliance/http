//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

// we need a pragma once for the circular includes required
// clangd's intellisense
#pragma once

#ifndef BOOST_HTTP_IMPL_PARSER_HPP
#define BOOST_HTTP_IMPL_PARSER_HPP

#include <boost/http/parser.hpp>
#include <boost/http/sink.hpp>
#include <boost/http/detail/type_traits.hpp>

namespace boost {
namespace http {
namespace detail {

// A wrapper that provides reference semantics for dynamic buffers
// while satisfying the dynamic_buffer concept
template<class DynamicBuffer>
class dynamic_buffer_ref
{
    DynamicBuffer* p_;

public:
    using const_buffers_type = typename DynamicBuffer::const_buffers_type;
    using mutable_buffers_type = typename DynamicBuffer::mutable_buffers_type;

    explicit
    dynamic_buffer_ref(DynamicBuffer& b) noexcept
        : p_(&b)
    {
    }

    std::size_t
    size() const noexcept
    {
        return p_->size();
    }

    std::size_t
    max_size() const noexcept
    {
        return p_->max_size();
    }

    std::size_t
    capacity() const noexcept
    {
        return p_->capacity();
    }

    const_buffers_type
    data() const noexcept
    {
        return p_->data();
    }

    mutable_buffers_type
    prepare(std::size_t n)
    {
        return p_->prepare(n);
    }

    void
    commit(std::size_t n)
    {
        p_->commit(n);
    }

    void
    consume(std::size_t n)
    {
        p_->consume(n);
    }
};

} // detail

template<class ElasticBuffer>
typename std::enable_if<
    ! detail::is_reference_wrapper<
        ElasticBuffer>::value &&
    ! is_sink<ElasticBuffer>::value>::type
parser::
set_body(
    ElasticBuffer&& eb)
{
    // If this goes off it means you are trying
    // to pass by lvalue reference. Use std::ref
    // instead.
    static_assert(
        ! std::is_reference<ElasticBuffer>::value,
        "Use std::ref instead of pass-by-reference");

    // Check ElasticBuffer type requirements
    static_assert(
        capy::is_DynamicBuffer<ElasticBuffer>::value,
        "Type requirements not met.");

    // body must not already be set
    if(is_body_set())
        detail::throw_logic_error();

    // headers must be complete
    if(! got_header())
        detail::throw_logic_error();

    auto& dyn = ws().emplace<
        capy::any_DynamicBuffer_impl<typename
            std::decay<ElasticBuffer>::type,
                buffers_N>>(std::forward<ElasticBuffer>(eb));

    set_body_impl(dyn);
}

template<class ElasticBuffer>
void
parser::
set_body(
    std::reference_wrapper<ElasticBuffer> eb)
{
    // Check ElasticBuffer type requirements
    static_assert(
        capy::is_DynamicBuffer<ElasticBuffer>::value,
        "Type requirements not met.");

    // body must not already be set
    if(is_body_set())
        detail::throw_logic_error();

    // headers must be complete
    if(! got_header())
        detail::throw_logic_error();

    // Use dynamic_buffer_ref to provide reference semantics
    auto& dyn = ws().emplace<
        capy::any_DynamicBuffer_impl<
            detail::dynamic_buffer_ref<ElasticBuffer>,
            buffers_N>>(eb.get());

    set_body_impl(dyn);
}

template<
    class Sink,
    class... Args,
    class>
Sink&
parser::
set_body(Args&&... args)
{
    // body must not already be set
    if(is_body_set())
        detail::throw_logic_error();

    // headers must be complete
    if(! got_header())
        detail::throw_logic_error();

    auto& s = ws().emplace<Sink>(
        std::forward<Args>(args)...);

    set_body_impl(s);
    return s;
}

} // http
} // boost

#endif
