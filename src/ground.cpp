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

GroundGenerator::GroundGenerator(const WorldSeed seed)
: mNoiseGenerator(seed)
{
}
float GroundGenerator::GetVerticalCoord(const vec2 &p) const
{
    return 10 * (mNoiseGenerator.Noise(p / 50.0f) + mNoiseGenerator.Noise(p / 250.0f));
}
GroundRenderer::GroundRenderer(const WorldSeed seed)
: mGenerator(seed)
{
}
#define COUNT_CHUNKROW_POINTS (COUNT_CHUNKROW_TILES + 1)
size_t GetOnChunkIndexFor(const size_t ix, const size_t iz)
{
    return ix * COUNT_CHUNKROW_POINTS + iz;
}
#define COUNT_GROUND_CHUNKRENDER_INDICES (6 * COUNT_CHUNKROW_TILES * COUNT_CHUNKROW_TILES)
#define COUNT_GROUND_CHUNKRENDER_VERTICES (COUNT_CHUNKROW_POINTS * COUNT_CHUNKROW_POINTS)
#define GROUND_VERTEXBUFFER_SIZE (COUNT_GROUND_CHUNKRENDER_VERTICES * sizeof(GroundRenderVertex))
#define GROUND_INDEXBUFFER_SIZE (COUNT_GROUND_CHUNKRENDER_INDICES * sizeof(GroundRenderIndex))
void GroundRenderer::PrepareForChunk(const ChunkID id)
{
    GroundRenderVertex *vertices;
    GroundRenderIndex *indices;
    GroundChunkRenderObj obj;

    {
        GLLock scopedLock = App::Instance().GetGLLock();

        obj.pVertexBuffer = App::Instance().GetGLManager()->AllocBuffer();
        obj.pIndexBuffer = App::Instance().GetGLManager()->AllocBuffer();

        glBindBuffer(GL_ARRAY_BUFFER, *(obj.pVertexBuffer));
        CHECK_GL();
        glBufferData(GL_ARRAY_BUFFER, GROUND_VERTEXBUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
        CHECK_GL();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *(obj.pIndexBuffer));
        CHECK_GL();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GROUND_INDEXBUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
        CHECK_GL();

        vertices = (GroundRenderVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        CHECK_GL();
        indices = (GroundRenderIndex *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        CHECK_GL();
    }

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

            p00 = vec3(x0, mGenerator.GetVerticalCoord(vec2(x0, z0)), z0);
            p_0 = vec3(x_, mGenerator.GetVerticalCoord(vec2(x_, z0)), z0);
            p0_ = vec3(x0, mGenerator.GetVerticalCoord(vec2(x0, z_)), z_);
            p10 = vec3(x1, mGenerator.GetVerticalCoord(vec2(x1, z0)), z0);
            p01 = vec3(x0, mGenerator.GetVerticalCoord(vec2(x0, z1)), z1);

            t = normalize(normalize(p00 - p_0) + normalize(p10 - p00));
            b = normalize(normalize(p00 - p01) + normalize(p0_ - p00));
            n = cross(t, b);

            i = GetOnChunkIndexFor(ix, iz);

            vertices[i].position = p00;
            vertices[i].normal = n;

            if (ix < COUNT_CHUNKROW_TILES && iz < COUNT_CHUNKROW_TILES)
            {
                indices[indexCount + 0] = GetOnChunkIndexFor(ix, iz);
                indices[indexCount + 1] = GetOnChunkIndexFor(ix, iz + 1);
                indices[indexCount + 2] = GetOnChunkIndexFor(ix + 1, iz + 1);

                indices[indexCount + 3] = GetOnChunkIndexFor(ix, iz);
                indices[indexCount + 4] = GetOnChunkIndexFor(ix + 1, iz + 1);
                indices[indexCount + 5] = GetOnChunkIndexFor(ix + 1, iz);

                indexCount += 6;
            }
        }
    }

    {
        GLLock scopedLock = App::Instance().GetGLLock();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *(obj.pIndexBuffer));
        CHECK_GL();
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        CHECK_GL();

        glBindBuffer(GL_ARRAY_BUFFER, *(obj.pVertexBuffer));
        CHECK_GL();
        glUnmapBuffer(GL_ARRAY_BUFFER);
        CHECK_GL();
    }

    {
        std::scoped_lock lock(mtxChunkRenderObjs);
        mChunkRenderObjs.emplace(id, obj);
    }
}
class GroundChunkRenderLoadJob: public LoadJob
{
    private:
        GroundRenderer *pRenderer;
        ChunkID chunkID;
    public:
        GroundChunkRenderLoadJob(GroundRenderer *p, const ChunkID &id)
        : pRenderer(p), chunkID(id)
        {
        }
        void Run(void)
        {
            pRenderer->PrepareForChunk(chunkID);
        }
};
GLfloat GroundRenderer::GetRenderDistance(void)
{
    Config config;
    App::Instance().GetConfig(config);

    return config.render.distance;
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

    GLfloat renderDistance = GetRenderDistance();

    vec3 position(0.0f, 0.0f, 0.0f);
    float x, z;
    for (x = position.x - renderDistance; x < (position.x + renderDistance); x += CHUNK_SIZE)
    {
        for (z = position.z - renderDistance; z < (position.z + renderDistance); z += CHUNK_SIZE)
        {
            ChunkID id = GetChunkID(x, z);
            loader.Add(new GroundChunkRenderLoadJob(this, id));
        }
    }
}
bool GroundRenderer::HasBuffersFor(const ChunkID id)
{
    std::scoped_lock lock(mtxChunkRenderObjs);

    return mChunkRenderObjs.find(id) != mChunkRenderObjs.end();
}
void GroundRenderer::UpdateBuffers(const vec3 &center)
{
    float x, z, dx, dz;

    int64_t r;
    int64_t chx, chz;
    ChunkID centerID = GetChunkID(center.x, center.z), id;
    GLfloat renderDistance = GetRenderDistance();

    /* Fill in nearby chunks, closest get priority.
       After updating a single chunk, immediatly return so
       that the center position can be reset.
     */
    for (r = 0; (r * CHUNK_SIZE) < renderDistance; r++)
    {
        for (chx = -r; chx < r; chx++)
        {
            id.x = centerID.x + chx;

            id.z = centerID.z + r;
            if (!HasBuffersFor(id))
            {
                PrepareForChunk(id);
                return;
            }
            id.z = centerID.z - r;
            if (!HasBuffersFor(id))
            {
                PrepareForChunk(id);
                return;
            }
        }
        for (chz = -r; chz < r; chz++)
        {
            id.z = centerID.z + chz;

            id.x = centerID.x + r;
            if (!HasBuffersFor(id))
            {
                PrepareForChunk(id);
                return;
            }
            id.x = centerID.x - r;
            if (!HasBuffersFor(id))
            {
                PrepareForChunk(id);
                return;
            }
        }
    }

    {
        std::scoped_lock lock(mtxChunkRenderObjs);

        // Drop far away chunks.
        auto it = mChunkRenderObjs.begin();
        while (it != mChunkRenderObjs.end())
        {
            ChunkID id = it->first;
            std::tie(x, z) = GetChunkCenter(id);

            dx = abs(x - center.x) - CHUNK_SIZE;
            dz = abs(z - center.z) - CHUNK_SIZE;
            if ((dx * dx + dz * dz) > renderDistance * renderDistance)
            {
                it = mChunkRenderObjs.erase(it);
            }
            else
                it++;
        }
    }
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

    GLfloat renderDistance = GetRenderDistance();

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
                    glBindBuffer(GL_ARRAY_BUFFER, *(mChunkRenderObjs.at(id).pVertexBuffer));
                    CHECK_GL();

                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *(mChunkRenderObjs.at(id).pIndexBuffer));
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
