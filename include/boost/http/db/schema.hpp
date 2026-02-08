//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef BOOST_HTTP_DB_SCHEMA_HPP
#define BOOST_HTTP_DB_SCHEMA_HPP

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <string_view>

namespace boost {
namespace http {
namespace db {

/** Bitwise flags describing column properties.

    Accumulated on a @ref field_t descriptor via
    builder-style member functions.
*/
enum field_flags : unsigned
{
    flag_none           = 0,
    flag_primary_key    = 1 << 0,
    flag_auto_increment = 1 << 1,
    flag_not_null       = 1 << 2,
    flag_unique         = 1 << 3,
    flag_indexed        = 1 << 4
};

/** Describe a single column mapped to a struct member.

    The member pointer is stored as a data member so
    that the natural syntax works without requiring
    angle brackets. The entire schema description is
    constexpr and available at compile time.

    @par Example
    @code
    field("id", &user::id).primary_key().auto_increment()
    field("email", &user::email).not_null().unique()
    @endcode

    @tparam T Member value type (e.g. `std::string`).
    @tparam C Containing class type (e.g. `user`).

    @see field, embed_t, has_one_t
*/
template <typename T, typename C>
struct field_t
{
    using value_type = T;
    using class_type = C;

    std::string_view name;
    T C::* pointer;
    unsigned flags = flag_none;

    /// Mark this column as the primary key.
    constexpr field_t& primary_key()    { flags |= flag_primary_key;    return *this; }

    /// Mark this column as auto-incrementing.
    constexpr field_t& auto_increment() { flags |= flag_auto_increment; return *this; }

    /// Mark this column as NOT NULL.
    constexpr field_t& not_null()       { flags |= flag_not_null;       return *this; }

    /// Mark this column as UNIQUE.
    constexpr field_t& unique()         { flags |= flag_unique;         return *this; }

    /// Mark this column as indexed.
    constexpr field_t& indexed()        { flags |= flag_indexed;        return *this; }

    /// Return true if this column is a primary key.
    constexpr bool is_primary_key()    const { return flags & flag_primary_key; }

    /// Return true if this column is auto-incrementing.
    constexpr bool is_auto_increment() const { return flags & flag_auto_increment; }

    /// Return true if this column is NOT NULL.
    constexpr bool is_not_null()       const { return flags & flag_not_null; }

    /// Return true if this column is UNIQUE.
    constexpr bool is_unique()         const { return flags & flag_unique; }

    /// Return true if this column is indexed.
    constexpr bool is_indexed()        const { return flags & flag_indexed; }

    /// Return the member value from an object.
    constexpr T const& get(C const& obj) const
    {
        return obj.*pointer;
    }

    /// Set the member value on an object.
    constexpr void set(C& obj, T const& value) const
    {
        obj.*pointer = value;
    }

    /// Set the member value on an object by move.
    constexpr void set(C& obj, T&& value) const
    {
        obj.*pointer = static_cast<T&&>(value);
    }
};

/** Create a field descriptor for a member pointer.

    Deduces `T` and `C` from the member pointer so
    template arguments are never needed at the call site.

    @par Example
    @code
    field("email", &user::email)
    field("id", &user::id).primary_key().auto_increment()
    @endcode

    @param name Column name in the database table.
    @param ptr  Pointer to the mapped data member.

    @return A @ref field_t descriptor for the member.

    @see field_t
*/
template <typename T, typename C>
constexpr auto field(std::string_view name, T C::* ptr)
    -> field_t<T, C>
{
    return { name, ptr };
}

/** Describe a nested struct flattened into the parent table.

    The nested type must provide its own `tag_invoke`
    overloads for @ref fields_t. A prefix is prepended
    to each nested column name to avoid collisions.

    @par Example
    @code
    // If user::addr is an address with field "street",
    // the resulting column is "addr_street".
    embed("addr_", &user::addr)
    @endcode

    @tparam T Nested struct type.
    @tparam C Containing class type.

    @see embed, field_t
*/
template <typename T, typename C>
struct embed_t
{
    using value_type = T;
    using class_type = C;

    std::string_view prefix;
    T C::* pointer;

    /// Return the nested struct from an object.
    constexpr T const& get(C const& obj) const
    {
        return obj.*pointer;
    }

    /// Set the nested struct on an object.
    constexpr void set(C& obj, T const& value) const
    {
        obj.*pointer = value;
    }

    /// Set the nested struct on an object by move.
    constexpr void set(C& obj, T&& value) const
    {
        obj.*pointer = static_cast<T&&>(value);
    }
};

/** Create an embedded field descriptor for a nested struct.

    @param prefix String prepended to nested column names.
    @param ptr    Pointer to the nested data member.

    @return An @ref embed_t descriptor.

    @see embed_t
*/
template <typename T, typename C>
constexpr auto embed(std::string_view prefix, T C::* ptr)
    -> embed_t<T, C>
{
    return { prefix, ptr };
}

/** Describe a one-to-one relationship to another table.

    The referenced type must provide its own
    `tag_invoke` overloads for @ref table_name_t
    and @ref fields_t.

    @par Example
    @code
    has_one("address_id", &user::addr)
    @endcode

    @tparam T Referenced struct type.
    @tparam C Containing class type.

    @see has_one, has_many_t
*/
template <typename T, typename C>
struct has_one_t
{
    using value_type = T;
    using class_type = C;

    std::string_view foreign_key;
    T C::* pointer;

    /// Return the related object from the parent.
    constexpr T const& get(C const& obj) const
    {
        return obj.*pointer;
    }

    /// Set the related object on the parent.
    constexpr void set(C& obj, T const& value) const
    {
        obj.*pointer = value;
    }

    /// Set the related object on the parent by move.
    constexpr void set(C& obj, T&& value) const
    {
        obj.*pointer = static_cast<T&&>(value);
    }
};

/** Create a one-to-one relationship descriptor.

    @param foreign_key Column name of the foreign key.
    @param ptr         Pointer to the related data member.

    @return A @ref has_one_t descriptor.

    @see has_one_t
*/
template <typename T, typename C>
constexpr auto has_one(std::string_view foreign_key, T C::* ptr)
    -> has_one_t<T, C>
{
    return { foreign_key, ptr };
}

/** Describe a one-to-many relationship.

    The child type has a member acting as the foreign
    key pointing back to the parent's primary key.
    The child's foreign key member pointer is stored
    so the library can build the appropriate JOIN or
    subquery.

    @par Example
    @code
    has_many(&user::posts, &post::user_id)
    @endcode

    @tparam Collection Container type in the parent
        (e.g. `std::vector< post >`).
    @tparam Parent     Parent class type.
    @tparam FK         Foreign key value type in the child.
    @tparam Child      Child class type.

    @see has_many, has_one_t
*/
template <typename Collection, typename Parent, typename FK, typename Child>
struct has_many_t
{
    using collection_type = Collection;
    using parent_type     = Parent;
    using child_type      = Child;
    using fk_value_type   = FK;

    Collection Parent::* pointer;
    FK Child::* foreign_key;
};

/** Create a one-to-many relationship descriptor.

    @param ptr Pointer to the collection member in the parent.
    @param fk  Pointer to the foreign key member in the child.

    @return A @ref has_many_t descriptor.

    @see has_many_t
*/
template <
    typename Collection, typename Parent,
    typename FK, typename Child>
constexpr auto has_many(
    Collection Parent::* ptr,
    FK Child::* fk)
    -> has_many_t<Collection, Parent, FK, Child>
{
    return { ptr, fk };
}

/** Tag type for retrieving the table name of a mapped type.

    Customize via `tag_invoke`:

    @par Example
    @code
    constexpr auto tag_invoke(
        db::table_name_t, user const&)
    {
        return "users";
    }
    @endcode

    @see fields_t, HasMapping
*/
struct table_name_t
{
    template <typename T>
    constexpr auto operator()(T const& v) const
    {
        return tag_invoke(*this, v);
    }
};

/** Tag type for retrieving the field descriptors of a mapped type.

    Customize via `tag_invoke`:

    @par Example
    @code
    constexpr auto tag_invoke(
        db::fields_t, user const&)
    {
        return std::tuple(
            db::field("id", &user::id)
                .primary_key().auto_increment(),
            db::field("email", &user::email)
                .not_null().unique(),
            db::field("name", &user::name));
    }
    @endcode

    @see table_name_t, HasMapping
*/
struct fields_t
{
    template <typename T>
    constexpr auto operator()(T const& v) const
    {
        return tag_invoke(*this, v);
    }
};

/// Customization point object for @ref table_name_t.
inline constexpr table_name_t table_name{};

/// Customization point object for @ref fields_t.
inline constexpr fields_t     fields{};

/** Concept for types with a complete schema mapping.

    A conforming type must provide `tag_invoke`
    overloads for both @ref table_name_t and
    @ref fields_t.

    @par Syntactic Requirements
    @li `tag_invoke( table_name, v )` is convertible
        to `std::string_view`.
    @li `tag_invoke( fields, v )` is a valid expression.

    @tparam T The type to check for a schema mapping.

    @see table_name_t, fields_t
*/
template <typename T>
concept HasMapping = requires(T const& v)
{
    { tag_invoke(table_name, v) } -> std::convertible_to<std::string_view>;
    { tag_invoke(fields,     v) };
};

namespace detail {

template <typename Tuple, typename F, std::size_t... Is>
constexpr void for_each_impl(
    Tuple const& t, F&& f, std::index_sequence<Is...>)
{
    (f(std::get<Is>(t)), ...);
}

} // namespace detail

/** Invoke a callable for each field in a mapped type.

    @param f Callable invoked as `f( field_descriptor )`
        for every field in `T`'s mapping.

    @tparam T A type satisfying @ref HasMapping.

    @see field_count
*/
template <HasMapping T, typename F>
constexpr void for_each_field(F&& f)
{
    constexpr auto fs = tag_invoke(fields, T{});
    detail::for_each_impl(
        fs,
        static_cast<F&&>(f),
        std::make_index_sequence<
            std::tuple_size_v<decltype(fs)>>{});
}

/** Return the number of fields in a mapped type.

    @tparam T A type satisfying @ref HasMapping.

    @return The compile-time field count.

    @see for_each_field
*/
template <HasMapping T>
constexpr std::size_t field_count()
{
    constexpr auto fs = tag_invoke(fields, T{});
    return std::tuple_size_v<decltype(fs)>;
}

} // namespace db
} // namespace http
} // namespace boost

#endif