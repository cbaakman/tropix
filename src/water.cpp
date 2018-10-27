#include <glm/gtc/type_ptr.hpp>

#include "water.hpp"
#include "error.hpp"
#include "shader.hpp"
#include "app.hpp"


struct WaterVertex
{
    GLfloat x, z;
};

typedef unsigned int WaterIndex;

#define WATERVERTEX_POSITION_INDEX 0


const char waterVertexShaderSrc[] = R"shader(
#version 150

uniform float time;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform vec3 center;

in vec2 position;

out VertexData
{
    vec3 position,
         normal;
} vertexOut;

vec3 GetPosition(float x, float z)
{
    vec3 p;

    p.x = floor(center.x) + x;
    p.z = floor(center.z) + z;
    p.y = 2.5 + 2.5 * sin(time + p.x / 25 + p.z / 25 + 1.0);

    return p;
}

void main()
{
    vec3 p00 = GetPosition(position.x, position.y),
         p_0 = GetPosition(position.x - 1.0, position.y),
         p0_ = GetPosition(position.x, position.y - 1.0),
         p10 = GetPosition(position.x + 1.0, position.y),
         p01 = GetPosition(position.x, position.y + 1.0);

    vertexOut.position = p00;
    vertexOut.normal = normalize(cross(p10 - p00, p0_ - p00) +
                                 cross(p0_ - p00, p_0 - p00) +
                                 cross(p_0 - p00, p01 - p00) +
                                 cross(p01 - p00, p10 - p00));

    gl_Position = projectionMatrix * viewMatrix * vec4(vertexOut.position, 1.0);
}
)shader",

waterFragmentShaderSrc[] = R"shader(
#version 150

uniform vec3 lightDirection;

in VertexData
{
    vec3 position,
         normal;
} vertexIn;

out vec4 fragColor;

void main()
{
    vec3 n = normalize(vertexIn.normal);

    float l = clamp(-dot(lightDirection, n), 0.0, 1.0);

    fragColor = vec4(0.0, 0.0, l, 1.0);
}

)shader";


size_t GetOnWaterIndexFor(const size_t ix, const size_t iz, const int countPointsWidth)
{
    return ix * countPointsWidth + iz;
}
struct Water2DGrid
{
    WaterVertex *mVertices;
    WaterIndex *mIndices;
    size_t countVertices,
           countIndices;

    Water2DGrid(const size_t r)
    {
        size_t countPointsWidth = 2 * r + 1,
               countSquaresWidth = 2 * r, ix, iz, i;

        countVertices = countPointsWidth * countPointsWidth;
        mVertices = new WaterVertex[countVertices];
        countIndices = 6 * countSquaresWidth * countSquaresWidth;
        mIndices = new WaterIndex[countIndices];

        i = 0;
        for (ix = 0; ix < countPointsWidth; ix++)
        {
            for (iz = 0; iz < countPointsWidth; iz++)
            {
                mVertices[GetOnWaterIndexFor(ix, iz, countPointsWidth)] = {float(ix) - float(r),
                                                                           float(iz) - float(r)};

                if (ix < countSquaresWidth && iz < countSquaresWidth)
                {
                    mIndices[i] = GetOnWaterIndexFor(ix, iz, countPointsWidth);
                    mIndices[i + 1] = GetOnWaterIndexFor(ix, iz + 1, countPointsWidth);
                    mIndices[i + 2] = GetOnWaterIndexFor(ix + 1, iz + 1, countPointsWidth);

                    mIndices[i + 3] = GetOnWaterIndexFor(ix, iz, countPointsWidth);
                    mIndices[i + 4] = GetOnWaterIndexFor(ix + 1, iz + 1, countPointsWidth);
                    mIndices[i + 5] = GetOnWaterIndexFor(ix + 1, iz, countPointsWidth);

                    i += 6;
                }
            }
        }
    }

    ~Water2DGrid(void)
    {
        delete[] mVertices;
        delete[] mIndices;
    }
};
void WaterRenderer::FillBuffers(const size_t distanceSquares)
{
    Water2DGrid grid(distanceSquares);

    glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
    CHECK_GL();
    glBufferData(GL_ARRAY_BUFFER, grid.countVertices * sizeof(WaterVertex), grid.mVertices, GL_STATIC_DRAW);
    CHECK_GL();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pIndexBuffer);
    CHECK_GL();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, grid.countIndices * sizeof(WaterIndex), grid.mIndices, GL_STATIC_DRAW);
    CHECK_GL();

    countIndices = grid.countIndices;
}
void WaterRenderer::TellInit(Loader &loader)
{
    pProgram = App::Instance().GetGLManager()->AllocShaderProgram();
    VertexAttributeMap attributes;
    attributes["position"] = WATERVERTEX_POSITION_INDEX;
    App::Instance().PushGL(new ShaderLoadJob(*pProgram, waterVertexShaderSrc, waterFragmentShaderSrc, attributes));

    Config config;
    App::Instance().GetConfig(config);

    pVertexBuffer = App::Instance().GetGLManager()->AllocBuffer();
    pIndexBuffer = App::Instance().GetGLManager()->AllocBuffer();
    FillBuffers((size_t)config.render.distance);
}
void WaterRenderer::Render(const mat4 &projection, const mat4 &view, const vec3 &center, const vec3 &lightDirection, const float time)
{
    glUseProgram(*pProgram);
    CHECK_GL();

    GLint projectionMatrixLocation,
          viewMatrixLocation,
          lightDirectionLocation,
          centerLocation,
          timeLocation;

    projectionMatrixLocation = glGetUniformLocation(*pProgram, "projectionMatrix");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(projectionMatrixLocation);
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, value_ptr(projection));
    CHECK_GL();

    viewMatrixLocation = glGetUniformLocation(*pProgram, "viewMatrix");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(viewMatrixLocation);
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, value_ptr(view));
    CHECK_GL();

    lightDirectionLocation = glGetUniformLocation(*pProgram, "lightDirection");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(lightDirectionLocation);
    glUniform3fv(lightDirectionLocation, 1, value_ptr(lightDirection));
    CHECK_GL();

    centerLocation = glGetUniformLocation(*pProgram, "center");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(centerLocation);
    glUniform3fv(centerLocation, 1, value_ptr(center));
    CHECK_GL();

    timeLocation = glGetUniformLocation(*pProgram, "time");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(timeLocation);
    glUniform1f(timeLocation, time);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, *pVertexBuffer);
    CHECK_GL();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pIndexBuffer);
    CHECK_GL();

    glEnableVertexAttribArray(WATERVERTEX_POSITION_INDEX);
    CHECK_GL();

    glVertexAttribPointer(WATERVERTEX_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
    CHECK_GL();

    glDrawElements(GL_TRIANGLES, countIndices, GL_UNSIGNED_INT, 0);
    CHECK_GL();
}
