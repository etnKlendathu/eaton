#include <catch2/catch.hpp>
#include "examples/example1.h"

TEST_CASE("Simple serialization/deserialization")
{
    test::Person origin;
    origin.email = "person@email.org";
    origin.id    = 42;
    origin.name  = "Person";

    auto check = [](const test::Person& item) {
        REQUIRE(42 == item.id);
        REQUIRE("person@email.org" == item.email);
        REQUIRE("Person" == item.name);
    };

    check(origin);

    SECTION("Serialization yaml")
    {
        std::string cnt = pack::yaml::serialize(origin);
        REQUIRE(!cnt.empty());
        INFO("yaml content:" << cnt);

        test::Person restored;
        pack::yaml::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization json")
    {
        std::string cnt = pack::json::serialize(origin);
        REQUIRE(!cnt.empty());

        test::Person restored;
        pack::json::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization zconfig")
    {
        std::string cnt = pack::zconfig::serialize(origin);
        REQUIRE(!cnt.empty());

        test::Person restored;
        pack::zconfig::deserialize(cnt, restored);

        check(restored);
    }

    SECTION("Serialization protobuf bin")
    {
        std::string cnt = pack::protobuf::serialize(origin);
        REQUIRE(!cnt.empty());

        test::Person restored;
        pack::protobuf::deserialize(cnt, restored);

        check(restored);
    }
}
