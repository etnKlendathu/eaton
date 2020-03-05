#include "examples/example3.h"
#include <catch2/catch.hpp>

TEST_CASE("Nested serialization/deserialization")
{
    test3::Item origin;
    origin.name  = "Item";
    origin.sub.exists = true;
    origin.sub.name = "subname";

    auto check = [](const test3::Item& item) {
        REQUIRE("Item" == item.name);
        REQUIRE(true == item.sub.exists);
        REQUIRE("subname" == item.sub.name);

    };

    check(origin);

    SECTION("Serialization yaml")
    {
        std::string cnt = pack::yaml::serialize(origin);
        REQUIRE(!cnt.empty());

        test3::Item restored;
        pack::yaml::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization json")
    {
        std::string cnt = pack::json::serialize(origin);
        REQUIRE(!cnt.empty());

        test3::Item restored;
        pack::json::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization zconfig")
    {
        std::string cnt = pack::zconfig::serialize(origin);
        REQUIRE(!cnt.empty());

        test3::Item restored;
        pack::zconfig::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization protobuf")
    {
        std::string cnt = pack::protobuf::serialize(origin);
        REQUIRE(!cnt.empty());

        test3::Item restored;
        pack::protobuf::deserialize(cnt, restored);

        check(restored);
    }
}
