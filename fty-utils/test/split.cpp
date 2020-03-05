#include "utils/split.h"
#include <catch2/catch.hpp>


TEST_CASE("Split utils")
{
    SECTION("Vector")
    {
        auto vec = fty::split("It's dead, that's what's wrong with it.", " ");
        CHECK(std::vector<std::string>{"It's", "dead,", "that's", "what's", "wrong", "with", "it."} == vec);
    }

    SECTION("Vector, skip empty")
    {
        auto vec = fty::split("this||is|an|ex-parrot|", "|");
        CHECK(std::vector<std::string>{"this", "is", "an", "ex-parrot"} == vec);
    }

    SECTION("Vector, nothing to split")
    {
        auto vec = fty::split("ex-parrot", "|");
        CHECK(std::vector<std::string>{"ex-parrot"} == vec);
    }

    SECTION("Vector, trim")
    {
        auto vec = fty::split("Norwegian    ,    Blue", ",");
        CHECK(std::vector<std::string>{"Norwegian", "Blue"} == vec);

        auto vec2 = fty::split("Norwegian    ,    Blue", ",", fty::SplitOption::NoTrim);
        CHECK(std::vector<std::string>{"Norwegian    ", "    Blue"} == vec2);
    }

    SECTION("Vector, split regex")
    {
        static std::regex re(",+");
        auto              vec = fty::split("this,,is,,,,an,,,ex-parrot", re, fty::SplitOption::KeepEmpty);
        CHECK(std::vector<std::string>{"this", "is", "an", "ex-parrot"} == vec);
    }

    SECTION("Tuple")
    {
        auto tuple = fty::split<std::string, int>("sense of life = 42", "=");
        CHECK(std::make_tuple("sense of life", 42) == tuple);
        auto [name, val] = tuple;
        CHECK("sense of life" == name);
        CHECK(42 == val);

        auto tuple2 = fty::split<std::string, int>("sense of life", "=");
        CHECK(std::make_tuple("sense of life", 0) == tuple2);

        auto tuple3 = fty::split<std::string, int>("sense of life = 42 = 66", "=");
        CHECK(std::make_tuple("sense of life", 42) == tuple3);
    }

    SECTION("Tuple, regex")
    {
        static std::regex re("=+");

        auto tuple = fty::split<std::string, int>("sense of life === 42", re);
        CHECK(std::make_tuple("sense of life", 42) == tuple);
        auto [name, val] = tuple;
        CHECK("sense of life" == name);
        CHECK(42 == val);

        auto tuple2 = fty::split<std::string, int>("sense of life", re);
        CHECK(std::make_tuple("sense of life", 0) == tuple2);

        auto tuple3 = fty::split<std::string, int>("sense of life = 42 === 66", re);
        CHECK(std::make_tuple("sense of life", 42) == tuple3);
    }
}
