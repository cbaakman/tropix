#ifndef WATER_HPP
#define WATER_HPP


#include <glm/glm.hpp>
using namespace glm;

#include "alloc.hpp"
#include "load.hpp"


#define WATER_WAVE_LENGTH 25.0f
#define WATER_WAVE_PERIOD 1.0f
#define WATER_WAVE_AMPLITUDE 2.5f

extern const std::string srcWaveFunc;


class WaterRenderer: public Initializable
{
    private:
        GLRef pProgram,
              pVertexBuffer, pIndexBuffer;
        size_t countIndices;

        void FillBuffers(const size_t distanceSquares);
    public:
        void TellInit(Queue &);

        void Render(const mat4 &projection, const mat4 &view, const vec3 &center, const vec3 &lightDirection, const float time);
};

#endif  // WATER_HPP
