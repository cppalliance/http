//
// Copyright (c) 2025 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/cppalliance/http
//

/** @file
    bcrypt password hashing library.

    This header provides bcrypt password hashing with three API tiers:

    **Tier 1 -- Synchronous** (low-level, no capy dependency):
    @code
    bcrypt::result r = bcrypt::hash("password", 12);
    std::error_code ec;
    bool ok = bcrypt::compare("password", r.str(), ec);
    @endcode

    **Tier 2 -- Capy Task** (lazy coroutine, caller controls executor):
    @code
    auto r = co_await bcrypt::hash_task("password", 12);
    @endcode

    **Tier 3 -- Friendly Async** (auto-offloads to system thread pool):
    @code
    auto r = co_await bcrypt::hash_async("password", 12);
    bool ok = co_await bcrypt::compare_async("password", r.str());
    @endcode
*/

#ifndef BOOST_HTTP_BCRYPT_HPP
#define BOOST_HTTP_BCRYPT_HPP

#include <boost/http/detail/config.hpp>
#include <boost/http/detail/except.hpp>
#include <boost/core/detail/string_view.hpp>

#include <boost/capy/continuation.hpp>
#include <boost/capy/task.hpp>
#include <boost/capy/ex/executor_ref.hpp>
#include <boost/capy/ex/io_env.hpp>
#include <boost/capy/ex/run_async.hpp>
#include <boost/capy/ex/system_context.hpp>

#include <cstddef>
#include <cstring>
#include <exception>
#include <string>
#include <system_error>

namespace boost {
namespace http {
namespace bcrypt {

//------------------------------------------------

/** bcrypt hash version prefix.

    The version determines which variant of bcrypt is used.
    All versions produce compatible hashes.
*/
enum class version
{
    /// $2a$ - Original specification
    v2a,

    /// $2b$ - Fixed handling of passwords > 255 chars (recommended)
    v2b
};

//------------------------------------------------

/** Error codes for bcrypt operations.

    These errors indicate malformed input from untrusted sources.
*/
enum class error
{
    /// Success
    ok = 0,

    /// Salt string is malformed
    invalid_salt,

    /// Hash string is malformed
    invalid_hash
};

} // bcrypt
} // http

} // boost

namespace std {
template<>
struct is_error_code_enum<
    ::boost::http::bcrypt::error>
    : std::true_type {};
} // std

namespace boost {
namespace http {
namespace bcrypt {

namespace detail {

struct BOOST_SYMBOL_VISIBLE
    error_cat_type
    : std::error_category
{
    BOOST_HTTP_DECL const char* name(
        ) const noexcept override;
    BOOST_HTTP_DECL std::string message(
        int) const override;
    constexpr error_cat_type() noexcept = default;
};

BOOST_HTTP_DECL extern
    error_cat_type error_cat;

} // detail

inline
std::error_code
make_error_code(
    error ev) noexcept
{
    return std::error_code{
        static_cast<std::underlying_type<
            error>::type>(ev),
        detail::error_cat};
}

//------------------------------------------------

/** Fixed-size buffer for bcrypt hash output.

    Stores a bcrypt hash string (max 60 chars) in an
    inline buffer with no heap allocation.

    @par Example
    @code
    bcrypt::result r = bcrypt::hash("password", 10);
    core::string_view sv = r;  // or r.str()
    std::cout << r.c_str();    // null-terminated
    @endcode
*/
class result
{
    char buf_[61];
    unsigned char size_;

public:
    /** Default constructor.

        Constructs an empty result.
    */
    result() noexcept
        : size_(0)
    {
        buf_[0] = '\0';
    }

    /** Return the hash as a string_view.
    */
    core::string_view
    str() const noexcept
    {
        return core::string_view(buf_, size_);
    }

    /** Implicit conversion to string_view.
    */
    operator core::string_view() const noexcept
    {
        return str();
    }

    /** Return null-terminated C string.
    */
    char const*
    c_str() const noexcept
    {
        return buf_;
    }

    /** Return pointer to data.
    */
    char const*
    data() const noexcept
    {
        return buf_;
    }

    /** Return size in bytes (excludes null terminator).
    */
    std::size_t
    size() const noexcept
    {
        return size_;
    }

    /** Check if result is empty.
    */
    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    /** Check if result contains valid data.
    */
    explicit
    operator bool() const noexcept
    {
        return size_ != 0;
    }

private:
    friend BOOST_HTTP_DECL result gen_salt(unsigned, version);
    friend BOOST_HTTP_DECL result hash(core::string_view, unsigned, version);
    friend BOOST_HTTP_DECL result hash(core::string_view, core::string_view, std::error_code&);

    char* buf() noexcept { return buf_; }
    void set_size(unsigned char n) noexcept
    {
        size_ = n;
        buf_[n] = '\0';
    }
};

//------------------------------------------------

/** Generate a random salt.

    Creates a bcrypt salt string suitable for use with
    the hash() function.

    @par Preconditions
    @code
    rounds >= 4 && rounds <= 31
    @endcode

    @par Exception Safety
    Strong guarantee.

    @par Complexity
    Constant.

    @param rounds Cost factor. Each increment doubles the work.
    Default is 10, which takes approximately 100ms on modern hardware.

    @param ver Hash version to use.

    @return A 29-character salt string.

    @throws std::invalid_argument if rounds is out of range.
    @throws system_error on RNG failure.
*/
BOOST_HTTP_DECL
result
gen_salt(
    unsigned rounds = 10,
    version ver = version::v2b);

/** Hash a password with auto-generated salt.

    Generates a random salt and hashes the password.

    @par Preconditions
    @code
    rounds >= 4 && rounds <= 31
    @endcode

    @par Exception Safety
    Strong guarantee.

    @par Complexity
    O(2^rounds).

    @param password The password to hash. Only the first 72 bytes
    are used (bcrypt limitation).

    @param rounds Cost factor. Each increment doubles the work.

    @param ver Hash version to use.

    @return A 60-character hash string.

    @throws std::invalid_argument if rounds is out of range.
    @throws system_error on RNG failure.
*/
BOOST_HTTP_DECL
result
hash(
    core::string_view password,
    unsigned rounds = 10,
    version ver = version::v2b);

/** Hash a password using a provided salt.

    Uses the given salt to hash the password. The salt should
    be a string previously returned by gen_salt() or extracted
    from a hash string.

    @par Exception Safety
    Strong guarantee.

    @par Complexity
    O(2^rounds).

    @param password The password to hash.

    @param salt The salt string (29 characters).

    @param ec Set to bcrypt::error::invalid_salt if the salt
    is malformed.

    @return A 60-character hash string, or empty result on error.
*/
BOOST_HTTP_DECL
result
hash(
    core::string_view password,
    core::string_view salt,
    std::error_code& ec);

/** Compare a password against a hash.

    Extracts the salt from the hash, re-hashes the password,
    and compares the result.

    @par Exception Safety
    Strong guarantee.

    @par Complexity
    O(2^rounds).

    @param password The plaintext password to check.

    @param hash The hash string to compare against.

    @param ec Set to bcrypt::error::invalid_hash if the hash
    is malformed.

    @return true if the password matches the hash, false if
    it does not match OR if an error occurred. Always check
    ec to distinguish between a mismatch and an error.
*/
BOOST_HTTP_DECL
bool
compare(
    core::string_view password,
    core::string_view hash,
    std::error_code& ec);

/** Extract the cost factor from a hash string.

    @par Exception Safety
    Strong guarantee.

    @par Complexity
    Constant.

    @param hash The hash string to parse.

    @param ec Set to bcrypt::error::invalid_hash if the hash
    is malformed.

    @return The cost factor (4-31) on success, or 0 if an
    error occurred.
*/
BOOST_HTTP_DECL
unsigned
get_rounds(
    core::string_view hash,
    std::error_code& ec);

namespace detail {

// bcrypt truncates passwords to 72 bytes
struct password_buf
{
    char data_[72];
    unsigned char size_;

    explicit password_buf(
        core::string_view s) noexcept
        : size_(static_cast<unsigned char>(
            (std::min)(s.size(), std::size_t{72})))
    {
        std::memcpy(data_, s.data(), size_);
    }

    operator core::string_view() const noexcept
    {
        return {data_, size_};
    }
};

// bcrypt hashes are always 60 characters
struct hash_buf
{
    char data_[61];
    unsigned char size_;

    explicit hash_buf(
        core::string_view s) noexcept
        : size_(static_cast<unsigned char>(
            (std::min)(s.size(), std::size_t{60})))
    {
        std::memcpy(data_, s.data(), size_);
        data_[size_] = '\0';
    }

    operator core::string_view() const noexcept
    {
        return {data_, size_};
    }
};

} // detail

//------------------------------------------------

/** Hash a password, returning a lazy task.

    Returns a @ref capy::task that wraps the synchronous
    hash() call. The caller can co_await this task directly
    or launch it on a specific executor via run_async().

    @par Example
    @code
    // co_await in current context
    bcrypt::result r = co_await bcrypt::hash_task("password", 12);

    // or launch on a specific executor
    run_async(my_executor)(bcrypt::hash_task("password", 12));
    @endcode

    @param password The password to hash.

    @param rounds Cost factor. Each increment doubles the work.

    @param ver Hash version to use.

    @return A lazy task yielding `result`.

    @throws std::invalid_argument if rounds is out of range.
    @throws system_error on RNG failure.
*/
inline
capy::task<result>
hash_task(
    core::string_view password,
    unsigned rounds = 10,
    version ver = version::v2b)
{
    detail::password_buf pw(password);
    co_return hash(pw, rounds, ver);
}

/** Compare a password against a hash, returning a lazy task.

    Returns a @ref capy::task that wraps the synchronous
    compare() call. Errors are translated to exceptions.

    @par Example
    @code
    bool ok = co_await bcrypt::compare_task("password", stored_hash);
    @endcode

    @param password The plaintext password to check.

    @param hash_str The hash string to compare against.

    @return A lazy task yielding `bool`.

    @throws system_error if the hash is malformed.
*/
inline
capy::task<bool>
compare_task(
    core::string_view password,
    core::string_view hash_str)
{
    detail::password_buf pw(password);
    detail::hash_buf hs(hash_str);
    std::error_code ec;
    bool ok = compare(pw, hs, ec);
    if(ec)
        http::detail::throw_system_error(ec);
    co_return ok;
}

//------------------------------------------------

namespace detail {

struct hash_async_op
{
    password_buf password_;
    unsigned rounds_;
    version ver_;
    result result_;
    std::exception_ptr ep_;
    capy::continuation cont_;

    bool await_ready() const noexcept
    {
        return false;
    }

    void await_suspend(
        std::coroutine_handle<void> cont,
        capy::io_env const* env)
    {
        cont_.h = cont;
        auto caller_ex = env->executor;
        auto& pool = capy::get_system_context();
        auto sys_ex = pool.get_executor();
        capy::run_async(sys_ex,
            [this, caller_ex]
            (result r) mutable
            {
                result_ = r;
                caller_ex.dispatch(cont_).resume();
            },
            [this, caller_ex]
            (std::exception_ptr ep) mutable
            {
                ep_ = ep;
                caller_ex.dispatch(cont_).resume();
            }
        )(hash_task(password_, rounds_, ver_));
    }

    result await_resume()
    {
        if(ep_)
            std::rethrow_exception(ep_);
        return result_;
    }
};

struct compare_async_op
{
    password_buf password_;
    hash_buf hash_str_;
    bool result_ = false;
    std::exception_ptr ep_;
    capy::continuation cont_;

    bool await_ready() const noexcept
    {
        return false;
    }

    void await_suspend(
        std::coroutine_handle<void> cont,
        capy::io_env const* env)
    {
        cont_.h = cont;
        auto caller_ex = env->executor;
        auto& pool = capy::get_system_context();
        auto sys_ex = pool.get_executor();
        capy::run_async(sys_ex,
            [this, caller_ex]
            (bool ok) mutable
            {
                result_ = ok;
                caller_ex.dispatch(cont_).resume();
            },
            [this, caller_ex]
            (std::exception_ptr ep) mutable
            {
                ep_ = ep;
                caller_ex.dispatch(cont_).resume();
            }
        )(compare_task(password_, hash_str_));
    }

    bool await_resume()
    {
        if(ep_)
            std::rethrow_exception(ep_);
        return result_;
    }
};

} // detail

/** Hash a password asynchronously on the system thread pool.

    Returns an awaitable that offloads the CPU-intensive
    bcrypt work to the system thread pool, then resumes
    the caller on their original executor. Modeled after
    Express.js: `await bcrypt.hash(password, 12)`.

    @par Example
    @code
    bcrypt::result r = co_await bcrypt::hash_async("my_password", 12);
    @endcode

    @param password The password to hash.

    @param rounds Cost factor. Each increment doubles the work.

    @param ver Hash version to use.

    @return An awaitable yielding `result`.

    @throws std::invalid_argument if rounds is out of range.
    @throws system_error on RNG failure.
*/
inline
detail::hash_async_op
hash_async(
    core::string_view password,
    unsigned rounds = 10,
    version ver = version::v2b)
{
    return detail::hash_async_op{
        detail::password_buf(password),
        rounds,
        ver,
        {},
        {},
        {}};
}

/** Compare a password against a hash asynchronously.

    Returns an awaitable that offloads the CPU-intensive
    bcrypt work to the system thread pool, then resumes
    the caller on their original executor. Modeled after
    Express.js: `await bcrypt.compare(password, hash)`.

    @par Example
    @code
    bool ok = co_await bcrypt::compare_async("my_password", stored_hash);
    @endcode

    @param password The plaintext password to check.

    @param hash_str The hash string to compare against.

    @return An awaitable yielding `bool`.

    @throws system_error if the hash is malformed.
*/
inline
detail::compare_async_op
compare_async(
    core::string_view password,
    core::string_view hash_str)
{
    return detail::compare_async_op{
        detail::password_buf(password),
        detail::hash_buf(hash_str),
        false,
        {},
        {}};
}

} // bcrypt
} // http
} // boost

#endif
