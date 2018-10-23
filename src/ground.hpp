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
    // Don't use GLRef here, because we want to release the buffers immediatly as the chunks are unloaded.
    GLuint mVertexBuffer,
           mIndexBuffer;
};

class GroundRenderer: public Initializable, public ChunkWorker
{
    private:
        std::recursive_mutex mtxChunkRenderObjs;
        std::unordered_map<ChunkID, GroundChunkRenderObj> mChunkRenderObjs;

        GLRef pProgram,
              pTexture;

        GroundGenerator mGenerator;
    public:
        GroundRenderer(const WorldSeed);

        void TellInit(Loader &loader);

        void Render(const mat4 &projection, const mat4 &view, const vec3 &center,
                    const vec4 &horizonColor, const vec3 &lightDirection);

        void PrepareFor(const ChunkID);
        void DestroyFor(const ChunkID);
        float GetWorkRadius(void) const;

    friend class GroundChunkRenderLoadJob;
};

#endif  // GROUND_HPP
