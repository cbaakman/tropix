#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "app.hpp"
#include "error.hpp"
#include "shader.hpp"
#include "ground.hpp"
#include "texture.hpp"


#define GROUND_POSITION_INDEX 0
#define GROUND_NORMAL_INDEX 1

const char groundVertexShaderSrc[] = R"shader(
#version 150

in vec3 position;
in vec3 normal;

out VertexData
{
    vec3 worldSpaceNormal;
    vec2 texCoords;
    float distance;
} vertexOut;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
    vertexOut.texCoords = position.xz / 5.0;
    vertexOut.worldSpaceNormal = normal;
    vertexOut.distance = -(viewMatrix * vec4(position, 1.0)).z;
}
)shader",
groundFragmentShaderSrc[] = R"shader(
#version 150

uniform sampler2D tex;

uniform vec3 lightDirection;
uniform vec4 horizonColor;
uniform float maxDistance;

const vec4 sunColor = vec4(0.8, 0.6, 0.3, 1.0);
const vec4 ambientColor = vec4(0.2, 0.4, 0.7, 1.0);

in VertexData
{
    vec3 worldSpaceNormal;
    vec2 texCoords;
    float distance;
} vertexIn;

out vec4 fragColor;


vec4 shade(vec4 light, vec4 color)
{
    return vec4(light.r * color.r, light.g * color.g, light.b * color.b, light.a * color.a);
}


void main()
{
    float d = clamp(vertexIn.distance, 0.0, maxDistance) / maxDistance;
    d = d * d;
    vec3 n = normalize(vertexIn.worldSpaceNormal);
    float l = clamp(-dot(lightDirection, n), 0.0, 1.0);
    vec4 texColor = texture(tex, vertexIn.texCoords);
    fragColor = (1 - d) * (l * shade(sunColor, texColor) + shade(ambientColor, texColor)) + d * horizonColor;
}
)shader";

GroundGridCalculator::GroundGridCalculator(const WorldSeed seed,
                                           const float r,
                                           const size_t s)
: SurfaceGridCalculator(r, s), mNoiseGenerator(seed)
{
}
void GroundRenderer::CenterPosition(const vec2 &p)
{
    float x = floor(p.x / mCalculator.GetQuadSize()),
          z = floor(p.y / mCalculator.GetQuadSize());

    mCalculator.origin.x = mCalculator.GetQuadSize() * x;
    mCalculator.origin.y = mCalculator.GetQuadSize() * z;
}
float GroundGridCalculator::GetAmplitude(const vec2 &p) const
{
    return 10 * (mNoiseGenerator.Noise((origin + p) / 50.0f) + mNoiseGenerator.Noise((origin + p) / 250.0f));
}
GroundRenderer::GroundRenderer(const WorldSeed seed, const GLfloat renderDist, const size_t renderSubdiv)
: mCalculator(seed, renderDist, renderSubdiv), renderDistance(renderDist)
{
}
void GroundRenderer::SizeBuffers(void)
{
    glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
    CHECK_GL();
    glBufferData(GL_ARRAY_BUFFER, mCalculator.CountPoints() * sizeof(GroundRenderVertex), NULL, GL_DYNAMIC_DRAW);
    CHECK_GL();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pIndexBuffer);
    CHECK_GL();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mCalculator.CountQuads() * sizeof(GroundRenderIndex), NULL, GL_DYNAMIC_DRAW);
    CHECK_GL();
}
void GroundRenderer::TellInit(Loader &loader)
{
    pTexture = App::Instance().GetGLManager()->AllocTexture();
    loader.Add(new PNGTextureLoadJob("sand", *pTexture));

    VertexAttributeMap attributes;
    attributes["position"] = GROUND_POSITION_INDEX;
    attributes["normal"] = GROUND_NORMAL_INDEX;
    pProgram = App::Instance().GetGLManager()->AllocShaderProgram();
    loader.Add(new ShaderLoadJob(*pProgram, groundVertexShaderSrc, groundFragmentShaderSrc, attributes));

    pVertexBuffer = App::Instance().GetGLManager()->AllocBuffer();
    pIndexBuffer = App::Instance().GetGLManager()->AllocBuffer();
    SizeBuffers();
}
void GroundRenderer::RenderStart(void)
{
    indexCount = 0;

    glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
    CHECK_GL();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pIndexBuffer);
    CHECK_GL();

    vertices = (GroundRenderVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();
    indices = (GroundRenderIndex *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();
}
void GroundRenderer::OnPoint(const size_t i, const SurfaceGridPoint &p)
{
    vertices[i].position.x = p.position.x + mCalculator.origin.x;
    vertices[i].position.y = p.position.y;
    vertices[i].position.z = p.position.z + mCalculator.origin.y;

    vertices[i].normal = p.normal;
}
void GroundRenderer::OnQuad(const size_t i0, const size_t i1, const size_t i2, const size_t i3)
{
    indices[indexCount + 0] = i0;
    indices[indexCount + 1] = i1;
    indices[indexCount + 2] = i2;

    indices[indexCount + 3] = i0;
    indices[indexCount + 4] = i2;
    indices[indexCount + 5] = i3;

    indexCount += 6;
}
void GroundRenderer::RenderEnd(const mat4 &projection, const mat4 &view,
                               const vec4 &horizonColor, const vec3 &lightDirection,
                               const GLuint texture)
{
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    CHECK_GL();
    glUnmapBuffer(GL_ARRAY_BUFFER);
    CHECK_GL();

    // Position
    glEnableVertexAttribArray(GROUND_POSITION_INDEX);
    CHECK_GL();
    glVertexAttribPointer(GROUND_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(GroundRenderVertex), 0);
    CHECK_GL();

    // Normal
    glEnableVertexAttribArray(GROUND_NORMAL_INDEX);
    CHECK_GL();
    glVertexAttribPointer(GROUND_NORMAL_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(GroundRenderVertex), (GLvoid *)sizeof(vec3));
    CHECK_GL();

    glUseProgram(*pProgram);
    CHECK_GL();

    GLint projectionMatrixLocation,
          viewMatrixLocation,
          horizonColorLocation,
          lightDirectionLocation,
          maxDistanceLocation;

    projectionMatrixLocation = glGetUniformLocation(*pProgram, "projectionMatrix");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(projectionMatrixLocation);

    viewMatrixLocation = glGetUniformLocation(*pProgram, "viewMatrix");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(viewMatrixLocation);

    horizonColorLocation = glGetUniformLocation(*pProgram, "horizonColor");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(horizonColorLocation);

    lightDirectionLocation = glGetUniformLocation(*pProgram, "lightDirection");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(lightDirectionLocation);

    maxDistanceLocation =  glGetUniformLocation(*pProgram, "maxDistance");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(maxDistanceLocation);

    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, value_ptr(projection));
    CHECK_GL();

    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, value_ptr(view));
    CHECK_GL();

    glUniform4fv(horizonColorLocation, 1, value_ptr(horizonColor));
    CHECK_GL();

    glUniform3fv(lightDirectionLocation, 1, value_ptr(lightDirection));
    CHECK_GL();

    glUniform1f(maxDistanceLocation, renderDistance);
    CHECK_GL();

    glActiveTexture(GL_TEXTURE0);
    CHECK_GL();

    glBindTexture(GL_TEXTURE_2D, texture);
    CHECK_GL();

    glEnable(GL_CULL_FACE);
    CHECK_GL();

    glDepthMask(GL_TRUE);
    CHECK_GL();

    glEnable(GL_DEPTH_TEST);
    CHECK_GL();

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    CHECK_GL();

    glDisableVertexAttribArray(GROUND_POSITION_INDEX);
    CHECK_GL();
    glDisableVertexAttribArray(GROUND_NORMAL_INDEX);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL();
}
void GroundRenderer::Render(const mat4 &projection, const mat4 &view,
                            const vec4 &horizonColor, const vec3 &lightDirection)
{
    RenderStart();
    mCalculator.Iter(this, this);
    RenderEnd(projection, view, horizonColor, lightDirection, *pTexture);
}
