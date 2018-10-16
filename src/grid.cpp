#include <iostream>

#include "grid.hpp"

size_t SurfaceGridCalculator::CountPoints(void) const
{
    const size_t n = subdiv + 1;
    return n * n;
}
size_t SurfaceGridCalculator::CountQuads(void) const
{
    return subdiv * subdiv;
}
vec3 CalculateSurfaceNormal(const vec3 &p00, const vec3 &p_0, const vec3 &p0_, const vec3 &p10, const vec3 &p01)
{
    vec3 n1 = normalize(cross(p01 - p00, p10 - p00)),
         n2 = normalize(cross(p10 - p00, p0_ - p00)),
         n3 = normalize(cross(p0_ - p00, p_0 - p00)),
         n4 = normalize(cross(p_0 - p00, p01 - p00));

    return normalize(n1 + n2 + n3 + n4);
}
vec3 CalculateSurfaceTangent(const vec3 &p00, const vec3 &p_0, const vec3 &p10)
{
    return normalize(normalize(p00 - p_0) + normalize(p10 - p00));
}
vec3 CalculateSurfaceBitangent(const vec3 &p00, const vec3 &p0_, const vec3 &p01)
{
    // The bitangent points in opposite direction from the z-axis.
    return normalize(normalize(p0_ - p00) + normalize(p00 - p01));
}
SurfaceGridCalculator::SurfaceGridCalculator(const float r, const size_t s)
: radius(r), subdiv(s)
{
}
float SurfaceGridCalculator::GetQuadSize(void) const
{
    return (radius * 2) / subdiv;
}
void SurfaceGridCalculator::Iter(SurfaceGridPointActuator *pPA, SurfaceGridQuadActuator *pQA) const
{
    const size_t n = subdiv + 1;
    const float stepSize = GetQuadSize();

    size_t ix, iz;
    float x_, z_, x0, z0, x1, z1;

    vec3 p_0, p0_, p00, p01, p10, p11;
    SurfaceGridPoint gp;

    for (ix = 0; ix < n; ix++)
    {
        x_ = -radius + (((float)ix) - 1) * stepSize;
        x0 = -radius + ix * stepSize;
        x1 = -radius + (ix + 1) * stepSize;

        for (iz = 0; iz < n; iz++)
        {
            z_ = -radius + (((float)iz) - 1) * stepSize;
            z0 = -radius + iz * stepSize;
            z1 = -radius + (iz + 1) * stepSize;

            //if (z0 * z0 + x0 * x0 > radius * radius)
            //    continue;

            p_0 = vec3(x_, GetAmplitude(vec2(x_, z0)), z0);
            p0_ = vec3(x0, GetAmplitude(vec2(x0, z_)), z_);
            p00 = vec3(x0, GetAmplitude(vec2(x0, z0)), z0);
            p10 = vec3(x1, GetAmplitude(vec2(x1, z0)), z0);
            p01 = vec3(x0, GetAmplitude(vec2(x0, z1)), z1);
            p11 = vec3(x1, GetAmplitude(vec2(x1, z1)), z1);

            if (pPA != NULL)
            {
                gp.position = p00;
                gp.normal = CalculateSurfaceNormal(p00, p_0, p0_, p10, p01);
                gp.tangent = CalculateSurfaceTangent(p00, p_0, p10);
                gp.bitangent = CalculateSurfaceBitangent(p00, p0_, p01);

                pPA->OnPoint(ix * n + iz, gp);
            }

            if (pQA != NULL && ix < subdiv && iz < subdiv)
            {
                pQA->OnQuad(ix * n + iz,
                            ix * n + (iz + 1),
                            (ix + 1) * n + (iz + 1),
                            (ix + 1) * n + iz);
            }
        }
    }
}
