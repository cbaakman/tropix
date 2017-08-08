/* Copyright (C) 2017 Coos Baakman
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef TERRAIN_H
#define TERRAIN_H

#include <stdlib.h>
#include <vector>
#include <list>
#include <algorithm>
#include <random>

#include "vec.h"


class TerrainType
{
};

class Solid : public TerrainType
{
};

class Powder : public TerrainType
{
};

class Liquid : public TerrainType
{
public:
    virtual float GetThickness(void) const = 0;
};

struct TerrainVertex
{
    vec3 pos;
};

typedef unsigned int TerrainIndex;

struct TerrainEdge
{
    TerrainIndex i1, i2;
};

typedef uint64_t WorldSeed;


template <int N>
class PointGenerator
{
public:
    virtual void GetPoints(const vec<N> &from, const vec<N> &to,
                           std::list<vec<N>> &points) const = 0;
};

template <int N>
class CubicPointGenerator : public PointGenerator<N>
{
public:
    CubicPointGenerator(const WorldSeed seed,
                        const int chance,
                        const GLfloat cubeSize);

    void GetPoints(const vec<N> &from, const vec<N> &to,
                   std::list<vec<N>> &points) const;
};

template <>
class CubicPointGenerator<2> : public PointGenerator<2>
{
private:
    WorldSeed mSeed;
    int mChanceDivider;
    GLfloat mCubeSize;
public:
    CubicPointGenerator(const WorldSeed seed,
                        const int chanceDivider,
                        const GLfloat cubeSize):
        mSeed(seed),
        mChanceDivider(chanceDivider),
        mCubeSize(cubeSize)
    {
    }

    void GetPoints(const vec2 &from, const vec2 &to,
                   std::list<vec2> &points) const
    {
        GLfloat xmin, xmax, ymin, ymax,
                x, y;
        WorldSeed cubeSeed;

        xmin = mCubeSize * floor(std::min(from.x, to.x) / mCubeSize);
        xmax = mCubeSize * ceil(std::max(from.x, to.x) / mCubeSize);
        ymin = mCubeSize * floor(std::min(from.y, to.y) / mCubeSize);
        ymax = mCubeSize * ceil(std::max(from.y, to.y) / mCubeSize);

        for (x = xmin; x < xmax; x += mCubeSize)
        {
            for (y = ymin; y < ymax; y += mCubeSize)
            {
                cubeSeed = mSeed;

                cubeSeed *= cubeSeed * 6364136223846793005L + 1442695040888963407L;
                cubeSeed += WorldSeed(x / mCubeSize);
                cubeSeed *= cubeSeed * 6364136223846793005L + 1442695040888963407L;
                cubeSeed += WorldSeed(y / mCubeSize);
                cubeSeed *= cubeSeed * 6364136223846793005L + 1442695040888963407L;
                cubeSeed += WorldSeed(x / mCubeSize);
                cubeSeed *= cubeSeed * 6364136223846793005L + 1442695040888963407L;
                cubeSeed += WorldSeed(y / mCubeSize);

                if ((cubeSeed >> 24) % WorldSeed(mChanceDivider) == 0);
                {
                    points.push_back({x + mCubeSize / 2, y + mCubeSize / 2});
                }
            }
        }
    }
};


template <int N>
class NoiseGenerator
{
public:
    virtual float Noise(const vec<N> &) const = 0;
};

inline float PerlinFade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}
inline float Lerp(float t, float a0, float a1)
{
    return a0 + t * (a1 - a0);
}
inline float PerlinGradient2D(WorldSeed _hash, const vec2 &dir)
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

    return Dot(grad2d[_hash & 0x0f], dir);
}
inline float PerlinGradient3D(WorldSeed _hash, const vec3 &dir)
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

    return Dot(grad3d[_hash & 0x0f], dir);
}

template <int N>
class PerlinNoiseGenerator : public NoiseGenerator<N>
{
public:
    PerlinNoiseGenerator(const WorldSeed seed);

    /**
     * Returns between -1.0f and 1.0f, 0.0f for integers
     */
    float Noise(const vec<N> &) const;
};

template <>
class PerlinNoiseGenerator<2> : public NoiseGenerator<2>
{
private:
    WorldSeed mPermutations[512];
private:
    void Reseed(const WorldSeed seed)
    {
        for (size_t i = 0; i < 256; i++)
        {
            mPermutations[i] = i;
        }

        std::shuffle(std::begin(mPermutations),
                     std::begin(mPermutations) + 256,
                     std::default_random_engine(seed));

        for (size_t i = 0; i < 256; i++)
        {
            mPermutations[i + 256] = mPermutations[i];
        }
    }
public:
    PerlinNoiseGenerator(const WorldSeed seed)
    {
        Reseed(seed);
    }

    /**
     * Returns between -1.0f and 1.0f, 0.0f for integers
     */
    float Noise(const vec2 &p) const
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
};

template <>
class PerlinNoiseGenerator<3> : public NoiseGenerator<3>
{
private:
    WorldSeed mPermutations[512];
private:
    void Reseed(const WorldSeed seed)
    {
        for (size_t i = 0; i < 256; i++)
        {
            mPermutations[i] = i;
        }

        std::shuffle(std::begin(mPermutations),
                     std::begin(mPermutations) + 256,
                     std::default_random_engine(seed));

        for (size_t i = 0; i < 256; i++)
        {
            mPermutations[i + 256] = mPermutations[i];
        }
    }
public:
    PerlinNoiseGenerator(const WorldSeed seed)
    {
        Reseed(seed);
    }

    /**
     * Returns between -1.0f and 1.0f, 0.0f for integers
     */
    float Noise(const vec3 &p) const
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
};

template <int N>
class OctaveNoiseGenerator : public NoiseGenerator<N>
{
private:
    const NoiseGenerator<N> *pChild;
    int nOctaves;
    float mPersistence;
public:
    OctaveNoiseGenerator(const NoiseGenerator<N> *p,
                         const int octaves,
                         const float persistence):
        pChild(p),
        nOctaves(octaves),
        mPersistence(persistence)
    {
    }

    float Noise(const vec<N> &v) const
    {
        float total = 0.0f,
              frequency = 1.0f,
              amplitude = 1.0f,
              maxValue = 0.0f;  // Used for normalizing result
        int i;

        for(i = 0; i < nOctaves; i++)
        {
            total += pChild->Noise(v * frequency) * amplitude;

            maxValue += amplitude;

            amplitude *= mPersistence;
            frequency *= 2;
        }

        return total / maxValue;
    }
};
#endif  // TERRAIN_H
