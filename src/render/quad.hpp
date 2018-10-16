#ifndef RENDERQUAD_HPP
#define RENDERQUAD_HPP

#include <GL/glew.h>
#include <GL/gl.h>

#include <glm/glm.hpp>
using namespace glm;

#include "../alloc.hpp"
#include "../load.hpp"


struct QuadVertex
{
    vec3 position;
    vec2 texCoords;
};

struct RenderQuad
{
    GLuint texture;
    QuadVertex vertices[4];
};

class QuadRenderer: public Initializable
{
    private:
        GLRef pProgram, pBuffer;
    public:
        void TellInit(Loader &);

        void Render(const mat4 &projection, const RenderQuad &);
};

#endif  // RENDERQUAD_HPP
