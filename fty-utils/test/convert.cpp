#include "fty/convert.h"
#include <catch2/catch.hpp>

using namespace Catch::literals;

TEST_CASE("Convert")
{
    CHECK("11" == fty::convert<std::string>(11));
    CHECK("22.22" == fty::convert<std::string>(22.22));
    CHECK("true" == fty::convert<std::string>(true));
    CHECK("false" == fty::convert<std::string>(false));
    CHECK("11" == fty::convert<std::string>(11ll));
    CHECK("11" == fty::convert<std::string>(11ul));
    CHECK("str" == fty::convert<std::string>("str"));

    CHECK(32 == fty::convert<int>(32.222));
    CHECK(1 == fty::convert<int>(true));

    CHECK(42 == fty::convert<int>("42"));
    CHECK(true == fty::convert<bool>("true"));
    CHECK(false == fty::convert<bool>("false"));
    CHECK(42.22_a == fty::convert<float>("42.22"));
    CHECK(42.22_a == fty::convert<double>("42.22"));
}
