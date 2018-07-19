
// Domiplan test
// copyright 2018 (c) Hajime UCHIMURA / @nikq
// all rights reserved
#include "common.hpp"

#include "TestUtilities.hpp"

#include <random.hpp>

#include <math.h>

namespace
{
const float epsilon = 0.000001f;
}

TEST_CASE("xor128", "")
{
    SECTION("average=0.5f,count=100")
    {
        RANDOM::xor128 r;

        double       sum   = 0.f;
        const int    count = 100;
        const double eps   = (double)1.f / log2(count);

        for (int i = 0; i < count; i++)
        {
            sum += r.rand01();
        }
        sum = sum / (double)count;
        printf("sum %f, eps %f\n", sum, eps);
        REQUIRE(sum == Approx(0.5f).margin(eps));
    }
    SECTION("average=0.5f,count=100000")
    {
        RANDOM::xor128 r;

        double       sum   = 0.f;
        const int    count = 100000;
        const double eps   = (double)1.f / log2(count);

        for (int i = 0; i < count; i++)
        {
            sum += r.rand01();
        }
        sum = sum / (double)count;
        printf("sum %f, eps %f\n", sum, eps);
        REQUIRE(sum == Approx(0.5f).margin(eps));
    }
}

TEST_CASE("xohiro256**", "")
{
    SECTION("average=0.5f,count=100")
    {
        RANDOM::xoshiro256aa r;

        double       sum   = 0.f;
        const int    count = 100;
        const double eps   = (double)1.f / log2(count);

        for (int i = 0; i < count; i++)
        {
            sum += r.rand01();
        }
        sum = sum / (double)count;
        printf("sum %f, eps %f\n", sum, eps);
        REQUIRE(sum == Approx(0.5f).margin(eps));
    }

    SECTION("average=0.5f,count=100000")
    {
        RANDOM::xoshiro256aa r;

        double       sum   = 0.f;
        const int    count = 100000;
        const double eps   = (double)1.f / log2(count);

        for (int i = 0; i < count; i++)
        {
            sum += r.rand01();
        }
        sum = sum / (double)count;

        printf("sum %f, eps %f\n", sum, eps);

        REQUIRE(sum == Approx(0.5f).margin(eps));
    }

    SECTION("average=0.5f,count=100000,seed=0")
    {
        RANDOM::xoshiro256aa r(0);

        double       sum   = 0.f;
        const int    count = 100000;
        const double eps   = (double)1.f / log2(count);

        for (int i = 0; i < count; i++)
        {
            sum += r.rand01();
        }
        sum = sum / (double)count;

        printf("sum %f, eps %f\n", sum, eps);

        REQUIRE(sum == Approx(0.5f).margin(eps));
    }
    SECTION("average=0.5f,count=100000,seed=1")
    {
        RANDOM::xoshiro256aa r(1);

        double       sum   = 0.f;
        const int    count = 100000;
        const double eps   = (double)1.f / log2(count);

        for (int i = 0; i < count; i++)
        {
            sum += r.rand01();
        }
        sum = sum / (double)count;

        printf("sum %f, eps %f\n", sum, eps);

        REQUIRE(sum == Approx(0.5f).margin(eps));
    }

    SECTION("average=0.5f,count=100000,seed=2")
    {
        RANDOM::xoshiro256aa r(2);

        double       sum   = 0.f;
        const int    count = 100000;
        const double eps   = (double)1.f / log2(count);

        for (int i = 0; i < count; i++)
        {
            sum += r.rand01();
        }
        sum = sum / (double)count;

        printf("sum %f, eps %f\n", sum, eps);

        REQUIRE(sum == Approx(0.5f).margin(eps));
    }
}