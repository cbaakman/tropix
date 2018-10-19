#ifndef GROUND_HPP
#define GROUND_HPP

#include <unordered_map>

#include <glm/glm.hpp>
using namespace glm;
#include <GL/glew.h>
#include <GL/gl.h>

#include "load.hpp"
#include "alloc.hpp"
#include "noise.hpp"
#include "chunk.hpp"


class GroundGenerator
{
    private:
        PerlinNoiseGenerator2D mNoiseGenerator;
    public:
        GroundGenerator(const WorldSeed);

        float GetVerticalCoord(const vec2 &coords) const;
};


struct GroundChunkRenderObj
{
    GLRef pVertexBuffer,
          pIndexBuffer;
};

class GroundRenderer: public Initializable
{
    private:
        GLfloat renderDistance;

        std::recursive_mutex mtxChunkRenderObjs;
        std::unordered_map<ChunkID, GroundChunkRenderObj> mChunkRenderObjs;

        GLRef pProgram,
              pTexture;

        GroundGenerator mGenerator;

        void PrepareForChunk(const ChunkID);
    public:
        GroundRenderer(const WorldSeed, const GLfloat renderDistance);

        void TellInit(Loader &loader);

        void Render(const mat4 &projection, const mat4 &view, const vec3 &center,
                    const vec4 &horizonColor, const vec3 &lightDirection);

        void UpdateBuffers(const vec3 &center);
        bool HasBuffersFor(const ChunkID);

    friend class GroundChunkRenderLoadJob;
};

#endif  // GROUND_HPP
