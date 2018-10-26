#ifndef GROUND_HPP
#define GROUND_HPP

#include <unordered_map>

#include <glm/glm.hpp>
using namespace glm;
#include <GL/glew.h>
#include <GL/gl.h>

#include "load.hpp"
#include "alloc.hpp"
#include "chunk.hpp"


class GroundGenerator
{
    private:
        PerlinNoiseGenerator2D mNoiseGenerator;
    public:
        GroundGenerator(const WorldSeed);

        float GetVerticalCoord(const vec2 &coords) const;
};

struct GroundRenderVertex
{
    vec3 position,
         normal;
};
typedef unsigned int GroundRenderIndex;

#define COUNT_CHUNKROW_POINTS (COUNT_CHUNKROW_TILES + 1)
#define COUNT_GROUND_CHUNKRENDER_INDICES (6 * COUNT_CHUNKROW_TILES * COUNT_CHUNKROW_TILES)
#define COUNT_GROUND_CHUNKRENDER_VERTICES (COUNT_CHUNKROW_POINTS * COUNT_CHUNKROW_POINTS)

struct GroundChunkRenderObj
{
    GroundRenderVertex vertices[COUNT_GROUND_CHUNKRENDER_VERTICES];
    GroundRenderIndex indices[COUNT_GROUND_CHUNKRENDER_INDICES];

    // Don't use GLRef here, because we want to release the buffers immediatly as the chunks are unloaded.
    GLuint mVertexBuffer,
           mIndexBuffer;
};

class GroundRenderer: public Initializable, public ChunkWorker
{
    private:
        std::recursive_mutex mtxChunkRenderObjs;
        std::unordered_map<ChunkID, GroundChunkRenderObj *> mChunkRenderObjs;

        GLRef pProgram,
              pTexture;

        void Set(const ChunkID, GroundChunkRenderObj *);
    public:
        ~GroundRenderer(void);

        void TellInit(Loader &loader);

        void Render(const mat4 &projection, const mat4 &view, const vec3 &center,
                    const vec4 &horizonColor, const vec3 &lightDirection);

        void PrepareFor(const ChunkID, const WorldSeed);
        void DestroyFor(const ChunkID);
        float GetWorkRadius(void) const;

    friend class GroundChunkRenderLoadJob;
    friend class GroundChunkBufferFillJob;
};

#endif  // GROUND_HPP
