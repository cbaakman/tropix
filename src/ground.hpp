#ifndef GROUND_HPP
#define GROUND_HPP

#include <glm/glm.hpp>
using namespace glm;

#include "load.hpp"
#include "alloc.hpp"
#include "noise.hpp"
#include "grid.hpp"


struct GroundRenderVertex
{
    vec3 position,
         normal;
};
typedef unsigned int GroundRenderIndex;

class GroundGridCalculator: public SurfaceGridCalculator
{
    private:
        vec2 origin;
        PerlinNoiseGenerator2D mNoiseGenerator;
    public:
        GroundGridCalculator(const WorldSeed, const float radius, const size_t subdiv);

        float GetAmplitude(const vec2 &coords) const;

    friend class GroundRenderer;
};

class GroundRenderer: public SurfaceGridPointActuator, SurfaceGridQuadActuator, Initializable
{
    private:
        GroundRenderVertex *vertices;
        GroundRenderIndex *indices;
        size_t indexCount;

        GLRef pIndexBuffer,
              pVertexBuffer,
              pProgram,
              pTexture;

        void SizeBuffers(void);

        GroundGridCalculator mCalculator;
        GLfloat renderDistance;

        void RenderStart(void);
        void OnPoint(const size_t i, const SurfaceGridPoint &);
        void OnQuad(const size_t i0, const size_t i1, const size_t i2, const size_t i3);
        void RenderEnd(const mat4 &projection, const mat4 &view,
                       const vec4 &horizonColor, const vec3 &lightDirection,
                       const GLuint texture);
    public:
        GroundRenderer(const WorldSeed, const GLfloat renderDistance, const size_t renderSubdiv);

        void TellInit(Loader &loader);

        void CenterPosition(const vec2 &p);
        void Render(const mat4 &projection, const mat4 &view,
                    const vec4 &horizonColor, const vec3 &lightDirection);
};

#endif  // GROUND_HPP
