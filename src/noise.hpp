#ifndef NOISE_HPP
#define NOISE_HPP

#include <glm/glm.hpp>
using namespace glm;


typedef uint64_t WorldSeed;

class NoiseGenerator2D
{
public:
    // Return value is between -1.0f and 1.0f. For integers: 0.0f
    virtual float Noise(const vec2 &) const = 0;
};

class NoiseGenerator3D
{
public:
    // Return value is between -1.0f and 1.0f. For integers: 0.0f
    virtual float Noise(const vec3 &) const = 0;
};

typedef WorldSeed Permutations[512];

class PerlinNoiseGenerator2D : public NoiseGenerator2D
{
private:
    Permutations mPermutations;
public:
    PerlinNoiseGenerator2D(const WorldSeed seed);

    void Reseed(const WorldSeed seed);

    float Noise(const vec2 &p) const;
};

class PerlinNoiseGenerator3D : public NoiseGenerator3D
{
private:
    Permutations mPermutations;
public:
    PerlinNoiseGenerator3D(const WorldSeed seed);

    void Reseed(const WorldSeed seed);

    float Noise(const vec3 &p) const;
};

#endif  // NOISE_HPP
