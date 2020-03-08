#include "examples/example5.h"
#include <catch2/catch.hpp>
#include <iostream>

TEST_CASE("Map serialization/deserialization")
{
    test5::Item origin;
    origin.name = "some name";

    auto& val = origin.intMap.append();
    val.key   = "key1";
    val.value = 42;

    auto& val2 = origin.intMap.append();
    val2.key   = "key2";
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

struct TestMap : public pack::Node
{
    using pack::Node::Node;

    pack::StringMap strs = FIELD("strs");
    pack::Int32Map  ints = FIELD("ints");

    META(TestMap, strs, ints)
};

TEST_CASE("Simple map serialization/deserialization")
{
    TestMap origin;
    origin.strs.append("key1", "some name1");
    origin.strs.append("key2", "some name2");

    origin.ints.append("key1", 12);
    origin.ints.append("key2", 13);

    auto check = [](const TestMap& item) {
        REQUIRE(2 == item.strs.size());
        REQUIRE(2 == item.ints.size());
        CHECK(item.strs.contains("key1"));
        CHECK(item.strs.contains("key2"));
        CHECK(item.ints.contains("key1"));
        CHECK(item.ints.contains("key2"));
        CHECK("some name1" == item.strs["key1"]);
        CHECK("some name2" == item.strs["key2"]);
        CHECK(12 == item.ints["key1"]);
        CHECK(13 == item.ints["key2"]);
    };

    check(origin);

    SECTION("Serialization yaml")
    {
        std::string cnt = pack::yaml::serialize(origin);
        REQUIRE(!cnt.empty());
        std::cerr << cnt << std::endl;

        TestMap restored;
        pack::yaml::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization json")
    {
        std::string cnt = pack::json::serialize(origin);
        REQUIRE(!cnt.empty());

        TestMap restored;
        pack::json::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization zconfig")
    {
        std::string cnt = pack::zconfig::serialize(origin);
        REQUIRE(!cnt.empty());

        TestMap restored;
        pack::zconfig::deserialize(cnt, restored);

        check(restored);
    }
}

