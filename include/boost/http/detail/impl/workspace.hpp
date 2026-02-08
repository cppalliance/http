//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

#ifndef BOOST_HTTP_DETAIL_IMPL_WORKSPACE_HPP
#define BOOST_HTTP_DETAIL_IMPL_WORKSPACE_HPP

#include <boost/config.hpp>

namespace boost {
namespace http {
namespace detail {

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4324) /* structure was padded due to __declspec(align()) */
#endif

struct workspace::any
{
    any* next = nullptr;

    BOOST_HTTP_DECL
    virtual ~any() = 0;
};

template<class U>
struct alignas(::max_align_t)
    workspace::any_impl : any
{
    U u;

    any_impl(any_impl const&) = delete;
    any_impl(any_impl&&) = delete;

    template<class... Args>
    explicit any_impl(Args&&... args)
        : u(std::forward<Args>(args)...)
    {
    }
};

struct workspace::undo
{
    explicit
    undo(workspace& ws0) noexcept
        : ws_(ws0)
        , head_(ws0.head_)
    {
    }

    ~undo()
    {
        if(head_)
            ws_.head_ = head_;
    }

    void
    commit() noexcept
    {
        head_ = nullptr;
    }

private:
    workspace& ws_;
    unsigned char* head_;
};

template<class T>
constexpr
std::size_t
workspace::
space_needed()
{
    using U = typename std::decay<T>::type;

    static_assert(
        alignof(U) <= alignof(::max_align_t),
        "Overaligned types not supported");

    return sizeof(any_impl<U>);
}

template<class T, class... Args>
auto
workspace::
emplace(Args&&... args) ->
    typename std::decay<T>::type&
{
    static_assert(
        alignof(T) <= alignof(::max_align_t),
        "Overaligned types not supported");

    using U = any_impl<typename
        std::decay<T>::type>;

    undo u(*this);
    auto prev_head = head_;
    head_ = bump_down(sizeof(U), alignof(U));
    auto p = ::new(head_) U(
        std::forward<Args>(args)...);
    u.commit();
    p->next = reinterpret_cast<
        any*>(prev_head);
    return p->u;
}

template<class T>
T*
workspace::
push_array(
    std::size_t n,
    T const& t)
{
    struct alignas(::max_align_t)
        U : any
    {
        std::size_t n_ = 0;

        U() = default;
        ~U()
        {
            for(std::size_t i = n_;
                    i-- > 0;)
                data()[i].~T();
        }

        U(  std::size_t n,
            T const& t)
            : U()
        {
            while(n_ < n)
            {
                new(&data()[n_]) T(t);
                ++n_;
            }
        }

        T* data() noexcept
        {
            return reinterpret_cast<
                T*>(this + 1);
        }
    };

    undo u(*this);
    auto prev_head = head_;
    head_ = bump_down(
        sizeof(U) + n * sizeof(T),
        alignof(::max_align_t));
    auto p = ::new(head_) U(n, t);
    u.commit();
    p->next = reinterpret_cast<
        any*>(prev_head);
    return p->data();
}

#if defined(BOOST_MSVC)
#pragma warning(pop) /* C4324 */
#endif

} // detail
} // http
} // boost

#endif
