#include <catch2/catch.hpp>
#include "examples/example5.h"

TEST_CASE("Map serialization/deserialization")
{
    test5::Item origin;
    origin.name = "some name";

    auto& val = origin.intMap.append();
    val.key = "key1";
    val.value = 42;

    auto& val2 = origin.intMap.append();
    val2.key = "key2";
    val2.value = 66;

    auto check = [](const test5::Item& item) {
        REQUIRE("some name" == item.name);
        REQUIRE(2 == item.intMap.size());
        REQUIRE("key1" == item.intMap[0].key);
        REQUIRE("key2" == item.intMap[1].key);
        REQUIRE(42 == item.intMap[0].value);
        REQUIRE(66 == item.intMap[1].value);
    };

    check(origin);

    SECTION("Serialization yaml")
    {
        std::string cnt = pack::yaml::serialize(origin);
        REQUIRE(!cnt.empty());

        test5::Item restored;
        pack::yaml::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization json")
    {
        std::string cnt = pack::json::serialize(origin);
        REQUIRE(!cnt.empty());

        test5::Item restored;
        pack::json::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization zconfig")
    {
        std::string cnt = pack::zconfig::serialize(origin);
        REQUIRE(!cnt.empty());

        test5::Item restored;
        pack::zconfig::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization protobuf bin")
    {
        std::string cnt = pack::protobuf::serialize(origin);
        REQUIRE(!cnt.empty());

        test5::Item restored;
        pack::protobuf::deserialize(cnt, restored);

        check(restored);
    }
}

