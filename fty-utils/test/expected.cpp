#include "utils/expected.h"
#include <catch2/catch.hpp>
#include <iostream>

struct St
{
    St()          = default;
    St(const St&) = delete;
    St& operator=(const St&) = delete;
    St(St&&)                 = default;

    bool func()
    {
        return true;
    }
};

TEST_CASE("Expected")
{
    SECTION("Expected")
    {
        auto it = Expected<int>(32);
        CHECK(it);
        CHECK(32 == *it);
    }

    SECTION("Unexpected")
    {
        Expected<int> it = unexpected("wrong");
        CHECK(!it);
        CHECK("wrong" == it.error());
    }

    SECTION("Return values")
    {
        auto func = []() -> Expected<St> {
            return St();
        };

        auto func2 = []() -> Expected<St> {
            return unexpected("wrong");
        };

        Expected<St> st = func();
        CHECK(st);
        CHECK(st->func());
        CHECK((*st).func());

        Expected<St> ust = func2();
        CHECK(!ust);
        CHECK("wrong" == ust.error());
    }

    SECTION("Return streamed unexpected")
    {
        auto func = []() -> Expected<St> {
            return unexpected() << "wrong " << 42;
        };

        Expected<St> st = func();
        CHECK(!st);
        std::cerr << st.error() << std::endl;
        CHECK("wrong 42" == st.error());
    }
}
