#include <catch2/catch.hpp>
#include "utils/expected.h"

TEST_CASE("Expected")
{
    auto it = Expected<int>(32);
}
