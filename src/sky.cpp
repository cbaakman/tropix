#include <iostream>

#include <glm/gtc/type_ptr.hpp>

#include "app.hpp"
#include "sky.hpp"
#include "error.hpp"
#include "shader.hpp"


const char skyVertexShaderSrc[] = R"shader(
#version 150

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

in vec3 position;

out vec3 onSpherePosition;

void main()
{
    gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
    onSpherePosition = position;
}
)shader",

skyFragmentShaderSrc[] = R"shader(
#version 150

uniform float heightAboveHorizon;
uniform float horizonDistance;
uniform vec4 skyColor;
uniform vec4 horizonColor;

in vec3 onSpherePosition;

out vec4 fragColor;

void main()
{
    float f = clamp(onSpherePosition.y + heightAboveHorizon / horizonDistance, 0.0, 1.0);
    f = sqrt(f);
    fragColor = (1.0 - f) * horizonColor + f * skyColor;
}
)shader";

struct SkyVertex
{
    vec3 position;
};

typedef unsigned int SkyIndex;

#define SKY_POSITION_INDEX 0

size_t CountSpherePoints(const size_t lattitudes, const size_t longitudes)
{
    return 2 + (lattitudes - 1) * longitudes;
}
size_t CountSphereTriangles(const size_t lattitudes, const size_t longitudes)
{
    return 2 * longitudes + (lattitudes - 2) * longitudes * 2;
}

void SetSkySphere(const GLuint vertexBuffer, const GLuint indexBuffer,
                  const float radius,
                  const size_t lattitudes, const size_t longitudes)
{
    size_t i, j, jnext,
           countIndices = 0,
           countPoints = CountSpherePoints(lattitudes, longitudes),
           countTriangles = CountSphereTriangles(lattitudes, longitudes);

    float phi, theta;

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    CHECK_GL();
    glBufferData(GL_ARRAY_BUFFER, countPoints * sizeof(SkyVertex), NULL, GL_STATIC_DRAW);
    CHECK_GL();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    CHECK_GL();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * countTriangles * sizeof(SkyIndex), NULL, GL_STATIC_DRAW);
    CHECK_GL();

    SkyVertex *vertices = (SkyVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();
    SkyIndex *indices = (SkyIndex *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    CHECK_GL();

    // south pole
    vertices[0].position.x = 0.0f;
    vertices[0].position.y = -radius;
    vertices[0].position.z = 0.0f;
    for (j = 0; j < longitudes; j++)
    {
        jnext = (j + 1) % longitudes;

        indices[countIndices + 0] = 0;
        indices[countIndices + 1] = 1 + jnext;
        indices[countIndices + 2] = 1 + j;
        countIndices += 3;
    }

    for (i = 0; (i + 1) < lattitudes; i++)
    {
        phi = pi<float>() * (-0.5f + float(i + 1) / lattitudes);

        for (j = 0; j < longitudes; j++)
        {
            theta = (2 * pi<float>() * j) / longitudes;
            jnext = (j + 1) % longitudes;

            vertices[1 + i * longitudes + j].position.x = radius * cos(theta) * cos(phi);
            vertices[1 + i * longitudes + j].position.y = radius * sin(phi);
            vertices[1 + i * longitudes + j].position.z = radius * sin(theta) * cos(phi);

            if ((i + 2) < lattitudes)
            {
                indices[countIndices + 0] = 1 + (i + 1) * longitudes + j;
                indices[countIndices + 1] = 1 + i * longitudes + jnext;
                indices[countIndices + 2] = 1 + (i + 1) * longitudes + jnext;

                indices[countIndices + 3] = 1 + (i + 1) * longitudes + j;
                indices[countIndices + 4] = 1 + i * longitudes + j;
                indices[countIndices + 5] = 1 + i * longitudes + jnext;
                countIndices += 6;
            }
        }
    }

    // north pole
    vertices[countPoints - 1].position.x = 0.0f;
    vertices[countPoints - 1].position.y = radius;
    vertices[countPoints - 1].position.z = 0.0f;
    for (j = 0; j < longitudes; j++)
    {
        jnext = (j + 1) % longitudes;

        indices[countIndices + 0] = countPoints - 1;
        indices[countIndices + 1] = 1 + (lattitudes - 2) * longitudes + j;
        indices[countIndices + 2] = 1 + (lattitudes - 2) * longitudes + jnext;
        countIndices += 3;
    }

    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    CHECK_GL();
    glUnmapBuffer(GL_ARRAY_BUFFER);
    CHECK_GL();
}

SkyRenderer::SkyRenderer(const size_t subdiv)
: countLattitudes(subdiv / 2),  // north pole excluded
  countLongitudes(subdiv)  // begin == end
{
}

void SkyRenderer::TellInit(Queue &)
{
    pSkyVertexBuffer = App::Instance().GetGLManager()->AllocBuffer();
    pSkyIndexBuffer = App::Instance().GetGLManager()->AllocBuffer();
    SetSkySphere(*pSkyVertexBuffer, *pSkyIndexBuffer, 1.0f, countLattitudes, countLongitudes);

    VertexAttributeMap attributes;
    attributes["position"] = SKY_POSITION_INDEX;
    pProgram = App::Instance().GetGLManager()->AllocShaderProgram();
    App::Instance().PushGL(new ShaderLoadJob(*pProgram, skyVertexShaderSrc, skyFragmentShaderSrc, attributes));
}

void SkyRenderer::Render(const mat4 &projection, const mat4 &view,
                         const float heightAboveHorizon,
                         const vec4 &horizonColor, const vec4 &skyColor)
{
    Config config;
    App::Instance().GetConfig(config);

    glDisable(GL_DEPTH_TEST);
    CHECK_GL();

    glDepthMask(GL_FALSE);
    CHECK_GL();

    glEnable(GL_CULL_FACE);
    CHECK_GL();

    glUseProgram(*pProgram);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, *pSkyVertexBuffer);
    CHECK_GL();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *pSkyIndexBuffer);
    CHECK_GL();

    glEnableVertexAttribArray(SKY_POSITION_INDEX);
    CHECK_GL();
    glVertexAttribPointer(SKY_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(SkyVertex), 0);
    CHECK_GL();

    GLint projectionMatrixLocation,
          viewMatrixLocation,
          heightAboveHorizonLocation,
          horizonDistanceLocation,
          horizonColorLocation,
          skyColorLocation;

    projectionMatrixLocation = glGetUniformLocation(*pProgram, "projectionMatrix");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(projectionMatrixLocation);

    viewMatrixLocation = glGetUniformLocation(*pProgram, "viewMatrix");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(viewMatrixLocation);

    heightAboveHorizonLocation = glGetUniformLocation(*pProgram, "heightAboveHorizon");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(heightAboveHorizonLocation);

    horizonDistanceLocation = glGetUniformLocation(*pProgram, "horizonDistance");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(horizonDistanceLocation);

    horizonColorLocation = glGetUniformLocation(*pProgram, "horizonColor");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(horizonColorLocation);

    skyColorLocation = glGetUniformLocation(*pProgram, "skyColor");
    CHECK_GL();
    CHECK_UNIFORM_LOCATION(skyColorLocation);

    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, value_ptr(projection));
    CHECK_GL();

    mat4 t = mat4(mat3(view));
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, value_ptr(t));
    CHECK_GL();

    glUniform1f(horizonDistanceLocation, config.render.distance);
    CHECK_GL();

    glUniform1f(heightAboveHorizonLocation, heightAboveHorizon);
    CHECK_GL();

    glUniform4fv(horizonColorLocation, 1, value_ptr(horizonColor));
    CHECK_GL();

    glUniform4fv(skyColorLocation, 1, value_ptr(skyColor));
    CHECK_GL();

    glDrawElements(GL_TRIANGLES, 3 * CountSphereTriangles(countLattitudes, countLongitudes), GL_UNSIGNED_INT, 0);
    CHECK_GL();

    glDisableVertexAttribArray(SKY_POSITION_INDEX);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL();
}
