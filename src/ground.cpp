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

GroundGenerator::GroundGenerator(const WorldSeed seed)
: mNoiseGenerator(seed)
{
}
float GroundGenerator::GetVerticalCoord(const vec2 &p) const
{
    return 10 * (mNoiseGenerator.Noise(p / 50.0f) + mNoiseGenerator.Noise(p / 250.0f));
}
size_t GetOnChunkIndexFor(const size_t ix, const size_t iz)
{
    return ix * COUNT_CHUNKROW_POINTS + iz;
}
#define GROUND_VERTEXBUFFER_SIZE (COUNT_GROUND_CHUNKRENDER_VERTICES * sizeof(GroundRenderVertex))
#define GROUND_INDEXBUFFER_SIZE (COUNT_GROUND_CHUNKRENDER_INDICES * sizeof(GroundRenderIndex))
class GroundChunkBufferFillJob: public Job
{
    private:
        ChunkID id;
        GroundRenderer *pRenderer;
        GroundChunkRenderObj *pObj;
    public:
        GroundChunkBufferFillJob(GroundRenderer *pR, const ChunkID cid, GroundChunkRenderObj *p)
        : pRenderer(pR), id(cid), pObj(p)
        {
        }

        void Run(void)
        {
            glGenBuffers(1, &(pObj->mVertexBuffer));
            CHECK_GL();

            glGenBuffers(1, &(pObj->mIndexBuffer));
            CHECK_GL();

            glBindBuffer(GL_ARRAY_BUFFER, pObj->mVertexBuffer);
            CHECK_GL();
            glBufferData(GL_ARRAY_BUFFER, GROUND_VERTEXBUFFER_SIZE, pObj->vertices, GL_DYNAMIC_DRAW);
            CHECK_GL();

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pObj->mIndexBuffer);
            CHECK_GL();
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, GROUND_INDEXBUFFER_SIZE, pObj->indices, GL_DYNAMIC_DRAW);
            CHECK_GL();

            pRenderer->Set(id, pObj);
        }
};
class GroundChunkBufferDeleteJob: public Job
{
    private:
        GroundChunkRenderObj *pObj;
    public:
        GroundChunkBufferDeleteJob(GroundChunkRenderObj *p)
        :pObj(p)
        {
        }

        void Run(void)
        {
            glDeleteBuffers(1, &(pObj->mVertexBuffer));
            CHECK_GL();

            glDeleteBuffers(1, &(pObj->mVertexBuffer));
            CHECK_GL();

            delete pObj;
        }

};
void GroundRenderer::Set(const ChunkID id, GroundChunkRenderObj *p)
{
    std::scoped_lock lock(mtxChunkRenderObjs);

    if (mChunkRenderObjs.find(id) != mChunkRenderObjs.end())
    {
        GroundChunkBufferDeleteJob deleteJob(mChunkRenderObjs.at(id));
        deleteJob.Run();

        mChunkRenderObjs.erase(id);
    }

    mChunkRenderObjs.emplace(id, p);
}
void GroundRenderer::PrepareFor(const ChunkID id, const WorldSeed seed)
{
    GroundGenerator groundGenerator(seed);

    GroundChunkRenderObj *pObj = new GroundChunkRenderObj;

    float ox = (float(id.x) - 0.5f) * CHUNK_SIZE,
          oz = (float(id.z) - 0.5f) * CHUNK_SIZE,
          x0, x_, x1, z0, z_, z1;

    vec3 p_0, p10, p0_, p01, p00, t, b, n;
    size_t ix, iz, i, indexCount = 0;

    for (ix = 0; ix < COUNT_CHUNKROW_POINTS; ix++)
    {
        x0 = ox + float(ix) * TILE_SIZE;
        x_ = ox + float(ix - 1) * TILE_SIZE;
        x1 = ox + float(ix + 1) * TILE_SIZE;

        for (iz = 0; iz < COUNT_CHUNKROW_POINTS; iz++)
        {
            z0 = oz + float(iz) * TILE_SIZE;
            z_ = oz + float(iz - 1) * TILE_SIZE;
            z1 = oz + float(iz + 1) * TILE_SIZE;

            p00 = vec3(x0, groundGenerator.GetVerticalCoord(vec2(x0, z0)), z0);
            p_0 = vec3(x_, groundGenerator.GetVerticalCoord(vec2(x_, z0)), z0);
            p0_ = vec3(x0, groundGenerator.GetVerticalCoord(vec2(x0, z_)), z_);
            p10 = vec3(x1, groundGenerator.GetVerticalCoord(vec2(x1, z0)), z0);
            p01 = vec3(x0, groundGenerator.GetVerticalCoord(vec2(x0, z1)), z1);

            t = normalize(normalize(p00 - p_0) + normalize(p10 - p00));
            b = normalize(normalize(p00 - p01) + normalize(p0_ - p00));
            n = cross(t, b);

            i = GetOnChunkIndexFor(ix, iz);

            pObj->vertices[i].position = p00;
            pObj->vertices[i].normal = n;

            if (ix < COUNT_CHUNKROW_TILES && iz < COUNT_CHUNKROW_TILES)
            {
                pObj->indices[indexCount + 0] = GetOnChunkIndexFor(ix, iz);
                pObj->indices[indexCount + 1] = GetOnChunkIndexFor(ix, iz + 1);
                pObj->indices[indexCount + 2] = GetOnChunkIndexFor(ix + 1, iz + 1);

                pObj->indices[indexCount + 3] = GetOnChunkIndexFor(ix, iz);
                pObj->indices[indexCount + 4] = GetOnChunkIndexFor(ix + 1, iz + 1);
                pObj->indices[indexCount + 5] = GetOnChunkIndexFor(ix + 1, iz);

                indexCount += 6;
            }
        }
    }

    App::Instance().PushGL(new GroundChunkBufferFillJob(this, id, pObj));
}
GroundRenderer::~GroundRenderer(void)
{
    for (const auto &pair : mChunkRenderObjs)
        DestroyFor(pair.first);
}
void GroundRenderer::DestroyFor(const ChunkID id)
{
    std::scoped_lock lock(mtxChunkRenderObjs);

    if (mChunkRenderObjs.find(id) != mChunkRenderObjs.end())
    {
        App::Instance().PushGL(new GroundChunkBufferDeleteJob(mChunkRenderObjs.at(id)));

        mChunkRenderObjs.erase(id);
    }
}
GLfloat GroundRenderer::GetWorkRadius(void) const
{
    Config config;
    App::Instance().GetConfig(config);

    return config.render.distance;
}
void GroundRenderer::TellInit(Queue &queue)
{
    pTexture = App::Instance().GetGLManager()->AllocTexture();
    queue.Add(new PNGTextureLoadJob("sand", *pTexture));

    VertexAttributeMap attributes;
    attributes["position"] = GROUND_POSITION_INDEX;
    attributes["normal"] = GROUND_NORMAL_INDEX;
    pProgram = App::Instance().GetGLManager()->AllocShaderProgram();
    App::Instance().PushGL(new ShaderLoadJob(*pProgram, groundVertexShaderSrc, groundFragmentShaderSrc, attributes));
}
void GroundRenderer::Render(const mat4 &projection, const mat4 &view, const vec3 &center,
                            const vec4 &horizonColor, const vec3 &lightDirection)
{
    glUseProgram(*pProgram);
    CHECK_GL();

    GLint projectionMatrixLocation,
          viewMatrixLocation,
          horizonColorLocation,
          lightDirectionLocation,
          horizonDistanceLocation;

    GLfloat renderDistance = GetWorkRadius();

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

    glUniform1f(horizonDistanceLocation, renderDistance);
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

    float x, z, dx, dz;
    for (x = center.x - renderDistance; x < (center.x + renderDistance); x += CHUNK_SIZE)
    {
        for (z = center.z - renderDistance; z < (center.z + renderDistance); z += CHUNK_SIZE)
        {
            dx = x - center.x;
            dz = z - center.z;
            if ((dx * dx + dz * dz) <= renderDistance * renderDistance)
            {
                ChunkID id = GetChunkID(x, z);

                std::scoped_lock lock(mtxChunkRenderObjs);

                if (mChunkRenderObjs.find(id) != mChunkRenderObjs.end())
                {
                    glBindBuffer(GL_ARRAY_BUFFER, mChunkRenderObjs.at(id)->mVertexBuffer);
                    CHECK_GL();

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mChunkRenderObjs.at(id)->mIndexBuffer);
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

                    glDrawElements(GL_TRIANGLES, COUNT_GROUND_CHUNKRENDER_INDICES, GL_UNSIGNED_INT, 0);
                    CHECK_GL();
                }
            }
        }
    }

    glDisableVertexAttribArray(GROUND_POSITION_INDEX);
    CHECK_GL();
    glDisableVertexAttribArray(GROUND_NORMAL_INDEX);
    CHECK_GL();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECK_GL();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    CHECK_GL();
}
