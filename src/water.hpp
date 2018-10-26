#ifndef WATER_HPP
#define WATER_HPP


#include <glm/glm.hpp>
using namespace glm;

#include "alloc.hpp"
#include "load.hpp"


class WaterRenderer: public Initializable
{
    private:
        GLRef pProgram,
              pVertexBuffer, pIndexBuffer;
        size_t countIndices;

        void FillBuffers(const size_t distanceSquares);
    public:
        void TellInit(Loader &);

        void Render(const mat4 &projection, const mat4 &view, const vec3 &center, const vec3 &lightDirection, const float time);
};

#endif  // WATER_HPP
