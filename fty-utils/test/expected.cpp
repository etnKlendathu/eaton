#include "utils/expected.h"
#include <catch2/catch.hpp>

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
    {
        auto it = Expected<int>(32);
        CHECK(it);
        CHECK(32 == *it);
    }
    {
        Expected<int> it = unexpected("wrong");
        CHECK(!it);
        CHECK("wrong" == it.error());
    }
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
}
