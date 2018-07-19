/*
* random
* copyright(c) 2018 Hajime UCHIMURA.
*/

#ifndef __RANDOM_H
#define __RANDOM_H

#include <algorithm>
#include <stdint.h>

namespace RANDOM
{
class xor128
{
  public:
    unsigned int x, y, z, w;
    xor128()
    {
        x = 123456789;
        y = 362436069;
        z = 521288629;
        w = 88675123;
    }
    inline unsigned int step(void)
    {
        unsigned int t;
        t        = x ^ (x << 11);
        x        = y;
        y        = z;
        z        = w;
        return w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
    }
    void setSeed(unsigned u)
    {
        x ^= u;
    }
    inline double rand01() { return (double)step() / (UINT_MAX); }
};

class xoshiro256aa
{
  private:
    /*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
     * To the extent possible under law, the author has dedicated all copyright
     * and related and neighboring rights to this software to the public domain
     * worldwide. This software is distributed without any warranty.
     * See <http://creativecommons.org/publicdomain/zero/1.0/>. */

    /* This is xoshiro256** 1.0, our all-purpose, rock-solid generator. It has
     * excellent (sub-ns) speed, a state (256 bits) that is large enough for
     * any parallel application, and it passes all tests we are aware of.
     * For generating just floating-point numbers, xoshiro256+ is even faster.
     * The state must be seeded so that it is not everywhere zero. If you have
     * a 64-bit seed, we suggest to seed a splitmix64 generator and use its output to fill s. */

    inline uint64_t rotl(const uint64_t x, int k) const
    {
        return (x << k) | (x >> (64 - k));
    }

    uint64_t s[4];

  public:
    xoshiro256aa(uint64_t seed = 1)
    {
        s[0] = std::hash<uint64_t>()(seed);
        s[1] = std::hash<uint64_t>()(seed + 1);
        s[2] = std::hash<uint64_t>()(seed + 2);
        s[3] = std::hash<uint64_t>()(seed + 3);
    }

    virtual ~xoshiro256aa() { ; }

    inline uint64_t next(void)
    {
        const uint64_t result_starstar = rotl(s[1] * 5, 7) * 9;

        const uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        return result_starstar;
    }

    inline double rand01()
    {
        uint64_t u = (next() >> 12) | (0x3FFull << 52);
        return *(double *)&u - 1.f;
    }

    /* This is the jump function for the generator. It is equivalent to 2^128 calls to next(); it can be used to generate 2^128 non-overlapping subsequences for parallel computations. */
    void jump(void)
    {
        static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};

        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;
        for (int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
            for (int b = 0; b < 64; b++)
            {
                if (JUMP[i] & UINT64_C(1) << b)
                {
                    s0 ^= s[0];
                    s1 ^= s[1];
                    s2 ^= s[2];
                    s3 ^= s[3];
                }
                next();
            }

        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;
    }

    /* This is the long-jump function for the generator. It is equivalent to 2^192 calls to next(); it can be used to generate 2^64 starting points,
     * from each of which jump() will generate 2^64 non-overlapping subsequences for parallel distributed computations. */
    void long_jump(void)
    {
        static const uint64_t LONG_JUMP[] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635};

        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        uint64_t s3 = 0;
        for (int i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
            for (int b = 0; b < 64; b++)
            {
                if (LONG_JUMP[i] & UINT64_C(1) << b)
                {
                    s0 ^= s[0];
                    s1 ^= s[1];
                    s2 ^= s[2];
                    s3 ^= s[3];
                }
                next();
            }

        s[0] = s0;
        s[1] = s1;
        s[2] = s2;
        s[3] = s3;
    }
};

} // namespace RANDOM

#endif
