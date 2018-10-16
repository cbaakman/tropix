#ifndef GROUND_HPP
#define GROUND_HPP

#include <glm/glm.hpp>
using namespace glm;

#include "load.hpp"
#include "alloc.hpp"
#include "noise.hpp"


class GroundGridCalculator
{
    private:
        PerlinNoiseGenerator2D mNoiseGenerator;
    public:
        GroundGridCalculator(const WorldSeed);

        float GetVerticalCoord(const vec2 &coords) const;

    friend class GroundRenderer;
};

class GroundRenderer: public Initializable
{
    private:
        GLfloat maxDist;
        size_t subdiv;

        GLRef pIndexBuffer,
              pVertexBuffer,
              pProgram,
              pTexture;

        size_t GetPointsPerRow(void) const;

        size_t GetIndexFor(const int ix, const int iz) const;

        float GetHorizontalCoord(const int i) const;

        void SizeBuffers(void);

        GroundGridCalculator mCalculator;
    public:
        GroundRenderer(const WorldSeed, const GLfloat renderDistance, const size_t renderSubdiv);

        void TellInit(Loader &loader);

        void Render(const mat4 &projection, const mat4 &view, const vec2 &center,
                    const vec4 &horizonColor, const vec3 &lightDirection);
};

#endif  // GROUND_HPP
