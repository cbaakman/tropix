#ifndef SKY_HPP
#define SKY_HPP

#include <glm/glm.hpp>
using namespace glm;

#include "alloc.hpp"
#include "load.hpp"


class SkyRenderer: Initializable
{
    private:
        size_t countLongitudes, countLattitudes;
        GLRef pSkyVertexBuffer, pSkyIndexBuffer,
              pProgram;
    public:
        SkyRenderer(const size_t subdiv);

        void TellInit(Loader &);

        void Render(const mat4 &projection, const mat4 &view, const vec4 &horizonColor, const vec4 &skyColor);
};


#endif  // SKY_HPP
