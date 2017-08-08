/* Copyright (C) 2017 Coos Baakman
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CHUNK_H
#define CHUNK_H

#include <map>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <vector>
#include <tuple>
#include <time.h>

#include <SDL2/SDL.h>

#include "terrain.h"

typedef int32_t ChunkCoord;

typedef int64_t ChunkID;

ChunkID MakeChunkID(const ChunkCoord x, const ChunkCoord z);
ChunkCoord GetX(const ChunkID &);
ChunkCoord GetZ(const ChunkID &);

#define PER_CHUNK_SIZE 16
#define MAX_WORLD_HEIGHT 100

struct Chunk
{
    std::vector<TerrainVertex> terrainVertices;
    std::vector<TerrainEdge> terrainEdges;
};

// Functions to convert coordinates:
vec2 Chunk2World(const ChunkID &, const vec2 &onChunk);
vec3 Chunk2World(const ChunkID &, const vec3 &onChunk);
vec2 World2Chunk(const ChunkID &, const vec2 &world);
vec3 World2Chunk(const ChunkID &, const vec3 &world);

std::tuple<void *, size_t> Serialize(const Chunk &);
void Deserialize(const void *data, const size_t size, Chunk &);

class ChunkGenerator
{
private:
    PerlinNoiseGenerator<2> *mPerlin;
    NoiseGenerator<3> *mNoiseGenerator;

    WorldSeed mSeed;
public:
    void Generate(const ChunkID, Chunk &) const;

    ChunkGenerator(const WorldSeed);
    ~ChunkGenerator(void);
};

class ChunkManager
{
private:
    SDL_mutex *mChunksMutex,
              *mRequestsMutex;

    ChunkGenerator *pGenerator;

    struct ChunkEntry
    {
        Chunk mChunk;
        time_t mLastAccess,
               mLastModify;

        ChunkEntry(void);
        ~ChunkEntry(void);
    };

    struct ChunkRequest
    {
        ChunkID id;
    };

    std::unordered_map<ChunkID, ChunkRequest> mRequests;
    std::unordered_map<ChunkID, ChunkEntry *> mEntries;

    bool bInitChunks;
    SDL_Thread *mChunkInitThread;
private:
    void TakeRequests(std::list<ChunkRequest> &);
public:
    ChunkManager(ChunkGenerator *);
    ~ChunkManager(void);

    void DeleteChunk(const ChunkID &);
    void GenerateChunk(const ChunkID &);
    void LoadChunk(const ChunkID &);

    void StartChunkInitThread(void);
    void StopChunkInitThread(void);

    void RequestChunk(const ChunkID &);
    void RequestChunks(const vec2 &center, const GLfloat radius);

    void DeleteChunksOlderThan(double seconds);

    bool HasChunk(const ChunkID &) const;

    bool GetChunkModificationTime(const ChunkID &, time_t &) const;
    bool GetChunk(const ChunkID &, Chunk &) const;
    bool SetChunk(const ChunkID &, const Chunk &);
};

#endif  // CHUNK_H
