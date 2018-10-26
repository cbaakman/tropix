#include <algorithm>
#include <array>
#include <random>
#include <cmath>

#include "noise.hpp"


float PerlinFade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}
float Lerp(float t, float a0, float a1)
{
    return a0 + t * (a1 - a0);
}
float PerlinGradient2D(WorldSeed _hash, const vec2 &dir)
{
    static vec2 grad2d[] = {{1.0f, 0.0f},
                            {0.9239f, 0.3827f},
                            {0.707107f, 0.707107f},
                            {0.3827f, 0.9239f},
                            {0.0f, 1.0f},
                            {-0.3827f, 0.9239f},
                            {-0.707107f, 0.707107f},
                            {-0.9239f, 0.3827f},
                            {-1.0f, 0.0f},
                            {-0.9239f, -0.3827f},
                            {-0.707107f, -0.707107f},
                            {-0.3827f, -0.9239f},
                            {0.0f, -1.0f},
                            {0.3827f, -0.9239f},
                            {0.707107f, -0.707107f},
                            {0.9239f, -0.3827f}};

    return dot(grad2d[_hash & 0x0f], dir);
}
float PerlinGradient3D(WorldSeed _hash, const vec3 &dir)
{
    static vec3 grad3d[] = {{1.0f, 1.0f, 0.0f},
                            {-1.0f, 1.0f, 0.0f},
                            {1.0f, -1.0f, 0.0f},
                            {-1.0f, -1.0f, 0.0f},
                            {1.0f, 0.0f, 1.0f},
                            {-1.0f, 0.0f, 1.0f},
                            {1.0f, 0.0f, -1.0f},
                            {-1.0f, 0.0f, -1.0f},
                            {0.0f, 1.0f, 1.0f},
                            {0.0f, -1.0f, 1.0f},
                            {0.0f, 1.0f, -1.0f},
                            {0.0f, -1.0f, -1.0f},
                            {1.0f, 1.0f, 0.0f},
                            {-1.0f, 1.0f, 0.0f},
                            {0.0f, -1.0f, 1.0f},
                            {0.0f, -1.0f, -1.0f}};

    return dot(grad3d[_hash & 0x0f], dir);
}
typedef WorldSeed Permutations[512];
void PerlinReseed(const WorldSeed seed, Permutations permutations)
{
    for (size_t i = 0; i < 256; i++)
    {
        permutations[i] = i;
    }

    std::shuffle(permutations,
                 permutations + 256,
                 std::default_random_engine(seed));

    for (size_t i = 0; i < 256; i++)
    {
        permutations[i + 256] = permutations[i];
    }
}
PerlinNoiseGenerator2D::PerlinNoiseGenerator2D(const WorldSeed seed)
{
    PerlinReseed(seed, mPermutations);
}
void PerlinNoiseGenerator2D::Reseed(const WorldSeed seed)
{
    PerlinReseed(seed, mPermutations);
}
float PerlinNoiseGenerator2D::Noise(const vec2 &p) const
{
    const WorldSeed X = WorldSeed(floor(p.x)) & 0xff,
                    Y = WorldSeed(floor(p.y)) & 0xff;

    float dx = p.x - floor(p.x),
          dy = p.y - floor(p.y);

    const float fx = PerlinFade(dx),
                fy = PerlinFade(dy);

    float grad00 = PerlinGradient2D(mPermutations[X + mPermutations[Y]], {dx, dy}),
          grad01 = PerlinGradient2D(mPermutations[X + mPermutations[Y + 1]], {dx, dy - 1.0f}),
          grad11 = PerlinGradient2D(mPermutations[X + 1 + mPermutations[Y + 1]], {dx - 1.0f, dy - 1.0f}),
          grad10 = PerlinGradient2D(mPermutations[X + 1 + mPermutations[Y]], {dx - 1.0f, dy});

    return Lerp(fy, Lerp(fx, grad00, grad10), Lerp(fx, grad01, grad11));
}
PerlinNoiseGenerator3D::PerlinNoiseGenerator3D(const WorldSeed seed)
{
    PerlinReseed(seed, mPermutations);
}
void PerlinNoiseGenerator3D::Reseed(const WorldSeed seed)
{
    PerlinReseed(seed, mPermutations);
}
float PerlinNoiseGenerator3D::Noise(const vec3 &p) const
{
    const WorldSeed X = WorldSeed(floor(p.x)) & 0xff;
    const WorldSeed Y = WorldSeed(floor(p.y)) & 0xff;
    const WorldSeed Z = WorldSeed(floor(p.z)) & 0xff;

    float dx = p.x - floor(p.x),
          dy = p.y - floor(p.y),
          dz = p.z - floor(p.z);

    const float fx = PerlinFade(dx);
    const float fy = PerlinFade(dy);
    const float fz = PerlinFade(dz);

    float grad000 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X] + Y] + Z], {dx, dy, dz}),
          grad100 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X + 1] + Y] + Z], {dx - 1.0f, dy, dz}),
          grad010 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X] + Y + 1] + Z], {dx, dy - 1.0f, dz}),
          grad110 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X + 1] + Y + 1] + Z], {dx - 1.0f, dy - 1.0f, dz}),
          grad001 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X] + Y] + Z + 1], {dx, dy, dz - 1.0f}),
          grad101 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X + 1] + Y] + Z + 1], {dx - 1.0f, dy, dz - 1.0f}),
          grad011 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X] + Y + 1] + Z + 1], {dx, dy - 1.0f, dz - 1.0f}),
          grad111 = PerlinGradient3D(mPermutations[mPermutations[mPermutations[X + 1] + Y + 1] + Z + 1], {dx - 1.0f, dy - 1.0f, dz - 1.0f});

    return Lerp(fz, Lerp(fy, Lerp(fx, grad000, grad100), Lerp(fx, grad010, grad110)),
                    Lerp(fy, Lerp(fx, grad001, grad101), Lerp(fx, grad011, grad111)));
}
template <class NoiseGenerator, typename Vec>
float OctaveNoise(const NoiseGenerator &child, const Vec &v, const float persistence, const size_t countOctaves)
{
    float total = 0.0f,
          frequency = 1.0f,
          amplitude = 1.0f,
          maxValue = 0.0f;  // Used for normalizing result
    size_t i;

    for(i = 0; i < countOctaves; i++)
    {
        total += child.Noise(v * frequency) * amplitude;

        maxValue += amplitude;

        amplitude *= persistence;
        frequency *= 2;
    }

    return total / maxValue;
}

