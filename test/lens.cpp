// copyright(c) 2018 Hajime UCHIMURA / nikq

#include "common.hpp"

#include "TestUtilities.hpp"

#include <lens.hpp>

#include <math.h>

#include <floatcanvas.hpp>

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
    SECTION("sag0")
    {
        Lens::Surface surface;
        surface.type_     = Lens::Surface::STANDARD; // for sphere
        surface.diameter_ = 1.;
        surface.curve_    = 1.;
        surface.center_   = 1.;
        surface.conic_    = 0.;
        surface.setup();

        Lens::Vector normal;
        double       sag_center = surface.sag(0.f, 0.f, normal);
        REQUIRE(sag_center == Approx(0.f).margin(eps));

        Lens::Vector expected = Lens::Vector(0.f, 0.f, -1.f).normal();
        REQUIRE_THAT(normal, IsApproxEquals(expected, eps));
    }

    SECTION("sag0.5")
    {
        Lens::Surface surface;
        surface.type_     = Lens::Surface::STANDARD; // for sphere
        surface.diameter_ = 1.;
        surface.curve_    = 1.;
        surface.center_   = 1.;
        surface.conic_    = 0.;
        surface.setup();

        Lens::Vector normal;
        double       sag = surface.sag(0.f, 1.f / sqrtf(2.f), normal);
        REQUIRE(sag == Approx(1.f - 1.f / sqrtf(2.f)).margin(eps));
        printf("normal %f,%f,%f\n", normal.x, normal.y, normal.z);
        Lens::Vector expected = Lens::Vector(0.f, 1.f, -1.f).normal();
        REQUIRE_THAT(normal, IsApproxEquals(expected, eps));
    }
}
