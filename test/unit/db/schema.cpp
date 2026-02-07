//
// Copyright (c) 2026 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

// Test that header file is self-contained.
#include <boost/http/db/schema.hpp>

#include <string>
#include <vector>

#include "test_suite.hpp"

namespace boost {
namespace http {
namespace db {

//----------------------------------------------------------
// Test fixtures
//----------------------------------------------------------

struct address
{
    std::string street;
    std::string city;
};

constexpr auto tag_invoke(table_name_t, address const&)
{
    return "addresses";
}

constexpr auto tag_invoke(fields_t, address const&)
{
    return std::tuple(
        field("street", &address::street),
        field("city",   &address::city));
}

struct post
{
    int         id = 0;
    int         user_id = 0;
    std::string title;
};

constexpr auto tag_invoke(table_name_t, post const&)
{
    return "posts";
}

constexpr auto tag_invoke(fields_t, post const&)
{
    return std::tuple(
        field("id",      &post::id).primary_key().auto_increment(),
        field("user_id", &post::user_id).not_null().indexed(),
        field("title",   &post::title));
}

struct user
{
    int                id = 0;
    std::string        email;
    std::string        name;
    address            addr;
    address            billing;
    std::vector<post>  posts;
};

constexpr auto tag_invoke(table_name_t, user const&)
{
    return "users";
}

constexpr auto tag_invoke(fields_t, user const&)
{
    return std::tuple(
        field("id",    &user::id).primary_key().auto_increment(),
        field("email", &user::email).not_null().unique(),
        field("name",  &user::name));
}

// Type with no mapping (for negative concept test)
struct unmapped {};

//----------------------------------------------------------

struct schema_test
{
    void
    test_field_flags()
    {
        BOOST_TEST(flag_none           == 0u);
        BOOST_TEST(flag_primary_key    == (1u << 0));
        BOOST_TEST(flag_auto_increment == (1u << 1));
        BOOST_TEST(flag_not_null       == (1u << 2));
        BOOST_TEST(flag_unique         == (1u << 3));
        BOOST_TEST(flag_indexed        == (1u << 4));

        // Flags combine correctly
        unsigned combined =
            flag_primary_key |
            flag_auto_increment |
            flag_not_null;
        BOOST_TEST(combined & flag_primary_key);
        BOOST_TEST(combined & flag_auto_increment);
        BOOST_TEST(combined & flag_not_null);
        BOOST_TEST(!(combined & flag_unique));
        BOOST_TEST(!(combined & flag_indexed));
    }

    void
    test_field_construction()
    {
        constexpr auto f = field("email", &user::email);

        BOOST_TEST(f.name == "email");
        BOOST_TEST(f.flags == flag_none);

        static_assert(
            std::is_same_v<
                decltype(f)::value_type,
                std::string>);
        static_assert(
            std::is_same_v<
                decltype(f)::class_type,
                user>);
    }

    void
    test_field_builder()
    {
        // Single flag
        {
            constexpr auto f =
                field("id", &user::id).primary_key();
            BOOST_TEST(f.is_primary_key());
            BOOST_TEST(!f.is_auto_increment());
            BOOST_TEST(!f.is_not_null());
            BOOST_TEST(!f.is_unique());
            BOOST_TEST(!f.is_indexed());
        }

        // Chained flags
        {
            constexpr auto f =
                field("id", &user::id)
                    .primary_key()
                    .auto_increment();
            BOOST_TEST(f.is_primary_key());
            BOOST_TEST(f.is_auto_increment());
            BOOST_TEST(!f.is_not_null());
        }

        // All flags
        {
            constexpr auto f =
                field("x", &user::id)
                    .primary_key()
                    .auto_increment()
                    .not_null()
                    .unique()
                    .indexed();
            BOOST_TEST(f.is_primary_key());
            BOOST_TEST(f.is_auto_increment());
            BOOST_TEST(f.is_not_null());
            BOOST_TEST(f.is_unique());
            BOOST_TEST(f.is_indexed());
        }
    }

    void
    test_field_get_set()
    {
        auto f_name = field("name", &user::name);
        auto f_id   = field("id",   &user::id);

        user u;
        u.name = "Alice";
        u.id   = 42;

        // get
        BOOST_TEST(f_name.get(u) == "Alice");
        BOOST_TEST(f_id.get(u) == 42);

        // set (lvalue)
        std::string new_name = "Bob";
        f_name.set(u, new_name);
        BOOST_TEST(u.name == "Bob");

        // set (rvalue)
        f_name.set(u, std::string("Carol"));
        BOOST_TEST(u.name == "Carol");

        // set int
        f_id.set(u, 99);
        BOOST_TEST(u.id == 99);
    }

    void
    test_embed_construction()
    {
        constexpr auto e =
            embed("addr_", &user::addr);

        BOOST_TEST(e.prefix == "addr_");

        static_assert(
            std::is_same_v<
                decltype(e)::value_type,
                address>);
        static_assert(
            std::is_same_v<
                decltype(e)::class_type,
                user>);
    }

    void
    test_embed_get_set()
    {
        auto e = embed("addr_", &user::addr);

        user u;
        u.addr.street = "123 Main St";
        u.addr.city   = "Springfield";

        // get
        BOOST_TEST(e.get(u).street == "123 Main St");
        BOOST_TEST(e.get(u).city   == "Springfield");

        // set (lvalue)
        address a2{"456 Oak Ave", "Shelbyville"};
        e.set(u, a2);
        BOOST_TEST(u.addr.street == "456 Oak Ave");
        BOOST_TEST(u.addr.city   == "Shelbyville");

        // set (rvalue)
        e.set(u, address{"789 Elm", "Capital City"});
        BOOST_TEST(u.addr.street == "789 Elm");
        BOOST_TEST(u.addr.city   == "Capital City");
    }

    void
    test_has_one()
    {
        constexpr auto h =
            has_one("billing_id", &user::billing);

        BOOST_TEST(h.foreign_key == "billing_id");

        static_assert(
            std::is_same_v<
                decltype(h)::value_type,
                address>);
        static_assert(
            std::is_same_v<
                decltype(h)::class_type,
                user>);

        user u;
        u.billing.street = "100 Invoice Lane";

        // get
        BOOST_TEST(h.get(u).street == "100 Invoice Lane");

        // set (lvalue)
        address a{"200 Pay St", "Billtown"};
        h.set(u, a);
        BOOST_TEST(u.billing.street == "200 Pay St");

        // set (rvalue)
        h.set(u, address{"300 Coin Rd", "Mintville"});
        BOOST_TEST(u.billing.city == "Mintville");
    }

    void
    test_has_many()
    {
        constexpr auto hm =
            has_many(&user::posts, &post::user_id);

        static_assert(
            std::is_same_v<
                decltype(hm)::collection_type,
                std::vector<post>>);
        static_assert(
            std::is_same_v<
                decltype(hm)::parent_type,
                user>);
        static_assert(
            std::is_same_v<
                decltype(hm)::child_type,
                post>);
        static_assert(
            std::is_same_v<
                decltype(hm)::fk_value_type,
                int>);
    }

    void
    test_table_name()
    {
        BOOST_TEST(std::string_view(table_name(user{}))    == "users");
        BOOST_TEST(std::string_view(table_name(address{})) == "addresses");
        BOOST_TEST(std::string_view(table_name(post{}))    == "posts");
    }

    void
    test_fields_cpo()
    {
        auto fs = fields(user{});
        BOOST_TEST(std::tuple_size_v<decltype(fs)> == 3u);
        BOOST_TEST(std::get<0>(fs).name == "id");
        BOOST_TEST(std::get<1>(fs).name == "email");
        BOOST_TEST(std::get<2>(fs).name == "name");

        // Verify flags survive the round trip
        BOOST_TEST(std::get<0>(fs).is_primary_key());
        BOOST_TEST(std::get<0>(fs).is_auto_increment());
        BOOST_TEST(std::get<1>(fs).is_not_null());
        BOOST_TEST(std::get<1>(fs).is_unique());
    }

    void
    test_has_mapping_concept()
    {
        static_assert(HasMapping<user>);
        static_assert(HasMapping<address>);
        static_assert(HasMapping<post>);
        static_assert(!HasMapping<unmapped>);
    }

    void
    test_for_each_field()
    {
        // Collect field names
        std::vector<std::string_view> names;
        for_each_field<user>(
            [&](auto const& f) { names.push_back(f.name); });

        BOOST_TEST(names.size() == 3u);
        BOOST_TEST(names[0] == "id");
        BOOST_TEST(names[1] == "email");
        BOOST_TEST(names[2] == "name");

        // Count flagged fields
        unsigned pk_count = 0;
        for_each_field<user>(
            [&](auto const& f)
            {
                if(f.is_primary_key())
                    ++pk_count;
            });
        BOOST_TEST(pk_count == 1u);
    }

    void
    test_field_count()
    {
        static_assert(field_count<user>()    == 3);
        static_assert(field_count<address>() == 2);
        static_assert(field_count<post>()    == 3);
    }

    void
    test_field_get_set_via_for_each()
    {
        user u;
        u.id    = 1;
        u.email = "alice@example.com";
        u.name  = "Alice";

        // Read every field via for_each_field
        std::vector<std::string> values;
        for_each_field<user>(
            [&](auto const& f)
            {
                auto const& v = f.get(u);
                if constexpr (std::is_same_v<
                    std::remove_cvref_t<decltype(v)>,
                    std::string>)
                    values.push_back(v);
            });

        BOOST_TEST(values.size() == 2u);
        BOOST_TEST(values[0] == "alice@example.com");
        BOOST_TEST(values[1] == "Alice");
    }

    void
    run()
    {
        test_field_flags();
        test_field_construction();
        test_field_builder();
        test_field_get_set();
        test_embed_construction();
        test_embed_get_set();
        test_has_one();
        test_has_many();
        test_table_name();
        test_fields_cpo();
        test_has_mapping_concept();
        test_for_each_field();
        test_field_count();
        test_field_get_set_via_for_each();
    }
};

TEST_SUITE(schema_test, "boost.http.db.schema");

} // namespace db
} // namespace http
} // namespace boost
