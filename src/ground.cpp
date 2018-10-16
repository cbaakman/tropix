#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "app.hpp"
#include "error.hpp"
#include "shader.hpp"
#include "ground.hpp"
#include "texture.hpp"


struct GroundRenderVertex
{
    vec3 position,
         normal;
};
typedef unsigned int GroundRenderIndex;

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
uniform float horizonDistance;

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
    float d = clamp(vertexIn.distance, 0.0, horizonDistance) / horizonDistance;
    vec3 n = normalize(vertexIn.worldSpaceNormal);
    float l = clamp(-dot(lightDirection, n), 0.0, 1.0);
    vec4 texColor = texture(tex, vertexIn.texCoords);
    fragColor = (1 - d) * (l * shade(sunColor, texColor) + shade(ambientColor, texColor)) + d * horizonColor;
}
)shader";

GroundGridCalculator::GroundGridCalculator(const WorldSeed seed)
: mNoiseGenerator(seed)
{
}
void GroundRenderer::CenterPosition(const vec2 &p)
{
    origin = p;
}
float GroundGridCalculator::GetVerticalCoord(const vec2 &p) const
{
    return 10 * (mNoiseGenerator.Noise(p / 50.0f) + mNoiseGenerator.Noise(p / 250.0f));
}
GroundRenderer::GroundRenderer(const WorldSeed seed, const GLfloat renderDist, const size_t renderSubdiv)
: mCalculator(seed), maxDist(renderDist), subdiv(renderSubdiv)
{
}
size_t GroundRenderer::GetPointsPerRow(void) const
{
    return 1 + 2 * subdiv;
}
void GroundRenderer::SizeBuffers(void)
{
    size_t pointsPerRow = GetPointsPerRow(),
           quadsPerRow = pointsPerRow - 1;
    glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
    CHECK_GL();
    glBufferData(GL_ARRAY_BUFFER, pointsPerRow * pointsPerRow * sizeof(GroundRenderVertex), NULL, GL_DYNAMIC_DRAW);
    CHECK_GL();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pIndexBuffer);
    CHECK_GL();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * quadsPerRow * quadsPerRow * sizeof(GroundRenderIndex), NULL, GL_DYNAMIC_DRAW);
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
float GroundRenderer::GetHorizontalCoord(const int i) const
{
    if (i < 0)
        return -exp(-i * log(maxDist) / subdiv);
    else if (i == 0)
        return 0.0f;
    else
        return exp(i * log(maxDist) / subdiv);
}
size_t GroundRenderer::GetIndexFor(const int ix, const int iz) const
{
    int n = GetPointsPerRow();

    return (subdiv + ix) * n + (subdiv + iz);
}
void GroundRenderer::Render(const mat4 &projection, const mat4 &view,
                            const vec4 &horizonColor, const vec3 &lightDirection)
{
    size_t indexCount = 0;

    glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
    CHECK_GL();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pIndexBuffer);
    CHECK_GL();

    GroundRenderVertex *vertices = (GroundRenderVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();
    GroundRenderIndex *indices = (GroundRenderIndex *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();

    vec3 p_0, p10, p0_, p01, p00, t, b, n;
    float x0, x_, x1, z0, z_, z1;
    signed int ix, iz,
               start = -subdiv, end = subdiv;

    for (ix = start; ix <= end; ix++)
    {
        x0 = GetHorizontalCoord(ix) + origin.x;
        x_ = GetHorizontalCoord(ix - 1) + origin.x;
        x1 = GetHorizontalCoord(ix + 1) + origin.x;

        for (iz = start; iz <= end; iz++)
        {
            z0 = GetHorizontalCoord(iz) + origin.y;
            z_ = GetHorizontalCoord(iz - 1) + origin.y;
            z1 = GetHorizontalCoord(iz + 1) + origin.y;

            p00 = vec3(x0, mCalculator.GetVerticalCoord(vec2(x0, z0)), z0);
            p_0 = vec3(x_, mCalculator.GetVerticalCoord(vec2(x_, z0)), z0);
            p0_ = vec3(x0, mCalculator.GetVerticalCoord(vec2(x0, z_)), z_);
            p10 = vec3(x1, mCalculator.GetVerticalCoord(vec2(x1, z0)), z0);
            p01 = vec3(x0, mCalculator.GetVerticalCoord(vec2(x0, z1)), z1);

            t = normalize(normalize(p00 - p_0) + normalize(p10 - p00));
            b = normalize(normalize(p00 - p01) + normalize(p0_ - p00));
            n = cross(t, b);

            vertices[GetIndexFor(ix, iz)].position = p00;
            vertices[GetIndexFor(ix, iz)].normal = n;

            if (ix < end && iz < end)
            {
                indices[indexCount + 0] = GetIndexFor(ix, iz);
                indices[indexCount + 1] = GetIndexFor(ix, iz + 1);
                indices[indexCount + 2] = GetIndexFor(ix + 1, iz + 1);

                indices[indexCount + 3] = GetIndexFor(ix, iz);
                indices[indexCount + 4] = GetIndexFor(ix + 1, iz + 1);
                indices[indexCount + 5] = GetIndexFor(ix + 1, iz);

                indexCount += 6;
            }
        }
    }

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
          horizonDistanceLocation;

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

    horizonDistanceLocation =  glGetUniformLocation(*pProgram, "horizonDistance");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(horizonDistanceLocation);

    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, value_ptr(projection));
    CHECK_GL();

    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, value_ptr(view));
    CHECK_GL();

    glUniform4fv(horizonColorLocation, 1, value_ptr(horizonColor));
    CHECK_GL();

    glUniform3fv(lightDirectionLocation, 1, value_ptr(lightDirection));
    CHECK_GL();

    glUniform1f(horizonDistanceLocation, 5000.0f);
    CHECK_GL();

    glActiveTexture(GL_TEXTURE0);
    CHECK_GL();

    glBindTexture(GL_TEXTURE_2D, *pTexture);
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
