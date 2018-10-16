#include <iostream>
#include <cstring>

#include <glm/gtc/type_ptr.hpp>
#include "quad.hpp"
#include "../error.hpp"
#include "../app.hpp"
#include "../shader.hpp"


#define VERTEX_POSITION_INDEX 0
#define VERTEX_TEXCOORDS_INDEX 1

const char quadVertexSrc[] = R"shader(
#version 150
in vec3 position;
in vec2 texCoords;

out VertexData
{
    vec2 texCoords;
} vertexOut;

uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * vec4(position, 1.0);
    vertexOut.texCoords = texCoords;
}
)shader",

quadFragmentSrc[] = R"shader(
#version 150

uniform sampler2D tex;

in VertexData
{
    vec2 texCoords;
} vertexIn;

out vec4 fragColor;
void main()
{
    fragColor = texture(tex, vertexIn.texCoords);
}
)shader";

void QuadSizeBuffer(const GLuint buffer)
{
    GLLock lock = App::Instance().GetGLLock();

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    CHECK_GL();

    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QuadVertex), NULL, GL_DYNAMIC_DRAW);
    CHECK_GL();
}
void QuadRenderer::TellInit(Loader &loader)
{
    pBuffer = App::Instance().GetGLManager()->AllocBuffer();
    QuadSizeBuffer(*pBuffer);

    pProgram = App::Instance().GetGLManager()->AllocShaderProgram();
    VertexAttributeMap attributes;
    attributes["position"] = VERTEX_POSITION_INDEX;
    attributes["texCoords"] = VERTEX_TEXCOORDS_INDEX;
    loader.Add(new ShaderLoadJob(*pProgram, quadVertexSrc, quadFragmentSrc, attributes));
}
void QuadRenderer::Render(const mat4 &projection, const RenderQuad &quad)
{
    glUseProgram(*pProgram);
    CHECK_GL();

    GLint loc = glGetUniformLocation(*pProgram, "projectionMatrix");
    CHECK_GL();
    if (loc < 0)
        throw GLError("quad projection matrix location not found");

    glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(projection));
    CHECK_GL();

    glActiveTexture(GL_TEXTURE0);
    CHECK_GL();

    glBindTexture(GL_TEXTURE_2D, quad.texture);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, *pBuffer);
    CHECK_GL();

    QuadVertex *p = (QuadVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();

    p[0] = quad.vertices[0];
    p[1] = quad.vertices[1];
    p[2] = quad.vertices[3];
    p[3] = quad.vertices[2];

    glUnmapBuffer(GL_ARRAY_BUFFER);
    CHECK_GL();

    // Position
    glEnableVertexAttribArray(VERTEX_POSITION_INDEX);
    CHECK_GL();
    glVertexAttribPointer(VERTEX_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), 0);
    CHECK_GL();

    // TexCoords
    glEnableVertexAttribArray(VERTEX_TEXCOORDS_INDEX);
    CHECK_GL();
    glVertexAttribPointer(VERTEX_TEXCOORDS_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (GLvoid *)sizeof(vec3));
    CHECK_GL();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CHECK_GL();

    glDisableVertexAttribArray(VERTEX_POSITION_INDEX);
    CHECK_GL();
    glDisableVertexAttribArray(VERTEX_TEXCOORDS_INDEX);
    CHECK_GL();
}
