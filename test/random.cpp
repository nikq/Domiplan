
// "colorsystem"
// copyright 2017 (c) Hajime UCHIMURA / @nikq
// all rights reserved
#include "common.hpp"

#include "TestUtilities.hpp"

#include <random.hpp>

namespace
{
const float epsilon = 0.000001f;
}

TEST_CASE("xor128", "")
{
    SECTION("average=0")
    {
        double sum = 0.f;

        RANDOM::xor128 r;

        for (int i = 0; i < 1000; i++)
        {
            sum += r.rand01() - 0.5f;
        }

        REQUIRE_THAT(sum, IsApproxEquals(0.f, epsilon));
    }
}
}