#include "common.hpp"

#include "TestUtilities.hpp"

#include <lens.hpp>

#include <math.h>

TEST_CASE("lens", "")
{
    const double eps = 1e-6;
    SECTION("reflect")
    {
        Lens::Vector in       = Lens::Vector(1.f, -1.f, 1.f).normal();
        Lens::Vector normal   = Lens::Vector(0.f, 1.f, 0.f).normal();
        Lens::Vector out      = Lens::reflect(in, normal);
        Lens::Vector expected = Lens::Vector(1.f, 1.f, 1.f).normal();
        REQUIRE_THAT(out, IsApproxEquals(expected, eps));
    }
}

TEST_CASE("loader", "")
{
    SECTION("ZMX")
    {
        Lens::Body lens = Lens::Loader::ZEMAX::load("test1.zmx");
        lens.dump();
    }
}