#include "examples/example6.h"
#include <catch2/catch.hpp>

TEST_CASE("Include serialization/deserialization")
{
    test6::Item origin;
    origin.name = "some name";
    origin.item.enval = test4::Item::EnumValue::Value2;

    auto check = [](const test6::Item& item) {
        REQUIRE("some name" == item.name);
        REQUIRE(test4::Item::EnumValue::Value2 == item.item.enval);
    };

    check(origin);

    SECTION("Serialization yaml")
    {
        std::string cnt = pack::yaml::serialize(origin);
        REQUIRE(!cnt.empty());

        test6::Item restored;
        pack::yaml::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization json")
    {
        std::string cnt = pack::json::serialize(origin);
        REQUIRE(!cnt.empty());

        test6::Item restored;
        pack::json::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization zconfig")
    {
        std::string cnt = pack::zconfig::serialize(origin);
        REQUIRE(!cnt.empty());

        test6::Item restored;
        pack::zconfig::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization protobuf")
    {
        std::string cnt = pack::protobuf::serialize(origin);
        REQUIRE(!cnt.empty());

        test6::Item restored;
        pack::protobuf::deserialize(cnt, restored);

        check(restored);
    }
}

