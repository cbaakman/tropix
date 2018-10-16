#ifndef GRID_HPP
#define GRID_HPP

#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
using namespace glm;


struct SurfaceGridPoint
{
    vec3 position, normal, tangent, bitangent;
};


class SurfaceGridPointActuator
{
    public:
        virtual void OnPoint(const size_t i, const SurfaceGridPoint &) = 0;
};


class SurfaceGridQuadActuator
{
    public:
        virtual void OnQuad(const size_t i0, const size_t i1, const size_t i2, const size_t i3) = 0;
};


/**
 *  Calculates gridpoints for user-defined amplitudes.
 */
class SurfaceGridCalculator
{
    private:
        float radius;
        size_t subdiv;
    public:
        virtual float GetAmplitude(const vec2 &coords) const = 0;

        SurfaceGridCalculator(const float radius, const size_t subdiv);

        float GetQuadSize(void) const;

        size_t CountPoints(void) const;
        size_t CountQuads(void) const;
        void Iter(SurfaceGridPointActuator *pPA, SurfaceGridQuadActuator *pQA=NULL) const;
};
#endif  // GRID_HPP
