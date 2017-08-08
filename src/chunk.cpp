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

#include <cstring>

#include "chunk.h"
#include "cube.h"
#include "log.h"
#include "exception.h"
#include "thread.h"


std::tuple<void *, size_t> Serialize(const Chunk &chunk)
{
    size_t sz = sizeof(TerrainVertex) * chunk.terrainVertices.size() +
                sizeof(TerrainEdge) * chunk.terrainEdges.size() +
                2 * sizeof(size_t),
           i = 0, j, n;
    void *data = malloc(sz);

    n = size_t(chunk.terrainVertices.size());
    memcpy(data + i, &n, sizeof(n));
    i += sizeof(n);

    for (j = 0; j < n; j++)
    {
        memcpy(data + i, &chunk.terrainVertices[j], sizeof(chunk.terrainVertices[j]));
        i += sizeof(chunk.terrainVertices[j]);
    }

    n = size_t(chunk.terrainEdges.size());
    memcpy(data + i, &n, sizeof(n));
    i += sizeof(n);

    for (j = 0; j < n; j++)
    {
        memcpy(data + i, &chunk.terrainEdges[j], sizeof(chunk.terrainEdges[j]));
        i += sizeof(chunk.terrainEdges[j]);
    }

    return std::make_tuple(data, sz);
}
void Deserialize(const void *data, const size_t sz, Chunk &chunk)
{
    size_t n, i = 0, j;
    TerrainVertex v;
    TerrainEdge e;

    chunk.terrainVertices.clear();
    memcpy(&n, data + i, sizeof(n));
    i += sizeof(n);
    chunk.terrainVertices.reserve(n);

    for (j = 0; j < n; j++)
    {
        memcpy(&v, data + i, sizeof(v));
        chunk.terrainVertices.push_back(v);
        i += sizeof(v);
    }

    chunk.terrainEdges.clear();
    memcpy(&n, data + i, sizeof(n));
    i += sizeof(n);
    chunk.terrainEdges.reserve(n);

    for (j = 0; j < n; j++)
    {
        memcpy(&e, data + i, sizeof(e));
        chunk.terrainEdges.push_back(e);
        i += sizeof(e);
    }
}
ChunkID MakeChunkID(const ChunkCoord x, const ChunkCoord z)
{
    return (ChunkID(x & 0xFFFFFFFF) << 32) | ChunkID(z & 0xFFFFFFFF);
}
ChunkCoord GetX(const ChunkID &id)
{
    return ChunkCoord(id >> 32);
}
ChunkCoord GetZ(const ChunkID &id)
{
    return ChunkCoord(id);
}
vec2 Chunk2World(const ChunkID &id, const vec2 &onChunk)
{
    return {GetX(id) * PER_CHUNK_SIZE + onChunk.x,
            GetZ(id) * PER_CHUNK_SIZE + onChunk.z};
}
vec2 World2Chunk(const ChunkID &id, const vec2 &world)
{
    return {world.x - GetX(id) * PER_CHUNK_SIZE,
            world.z - GetZ(id) * PER_CHUNK_SIZE};
}
vec3 Chunk2World(const ChunkID &id, const vec3 &onChunk)
{
    vec2 world = Chunk2World(id, {onChunk.x, onChunk.z});
    return {world.x, onChunk.y, world.z};
}
vec3 World2Chunk(const ChunkID &id, const vec3 &world)
{
    vec2 onChunk = World2Chunk(id, {world.x, world.z});
    return {onChunk.x, world.y, onChunk.z};
}
ChunkGenerator::ChunkGenerator(const WorldSeed seed):
    mSeed(seed)
{
    mPerlin = new PerlinNoiseGenerator<2>(seed);
    mNoiseGenerator = new PerlinNoiseGenerator<3>(seed);
}
ChunkGenerator::~ChunkGenerator(void)
{
    delete mPerlin;
    delete mNoiseGenerator;
}
template <int N>
bool PointBetween(const vec<N> &p1, const GLfloat a1,
                  const vec<N> &p2, const GLfloat a2,
                  const GLfloat value,
                  vec<N> &between)
{
    GLfloat f;

    if (value == a1 && value == a2)
    {
        f = 0.5f;
        between = p1 + f * (p2 - p1);
        return true;
    }
    else if (value >= a1 && value <= a2)
    {
        f = (value - a1) / (a2 - a1);
        between = p1 + f * (p2 - p1);
        return true;
    }
    else if (value >= a2 && value <= a1)
    {
        f = (value - a2) / (a1 - a2);
        between = p2 + f * (p1 - p2);
        return true;
    }
    else
        return false;
}
/*void ChunkGenerator::Generate(const ChunkID id, Chunk &chunk) const
{
    size_t w = PER_CHUNK_SIZE + 1;
    GLfloat height, land, landHeight, oceanHeight;
    vec2 p;

    TerrainIndex i = 0;
    TerrainIndex *indices = new TerrainIndex[w * w];

    chunk.terrainVertices.clear();
    chunk.terrainEdges.clear();

    for (int x = 0; x <= PER_CHUNK_SIZE; x++)
    {
        for (int z = 0; z <= PER_CHUNK_SIZE; z++)
        {
            p.x = GetX(id) * PER_CHUNK_SIZE + x;
            p.z = GetZ(id) * PER_CHUNK_SIZE + z;

            land = 0.5f + 0.5f * tanh(10.0f * mPerlin->Noise(p / 300) - 0.5f);

            landHeight = 5.0f + 3.0f * mPerlin->Noise(p / 40);
            oceanHeight = -20.0f + 3.0f * mPerlin->Noise(p / 10) + 50.0f * mPerlin->Noise(p / 50);

            height = landHeight * land + oceanHeight * (1.0f - land);

            chunk.terrainVertices.push_back({{x, height, z}});
            indices[x + w * z] = i;

            if (z > 0)
            {
                chunk.terrainEdges.push_back({indices[x + w * (z - 1)], i});
            }

            if (x > 0)
            {
                chunk.terrainEdges.push_back({indices[(x - 1) + w * z], i});
            }

            i ++;
        }
    }

    delete[] indices;
}
*/
/*
GLfloat GetHeight(const PerlinNoiseGenerator<2> *pPerlin, const vec2 &p)
{
    return pPerlin->Noise(p / 50) + pPerlin->Noise(p / 250);
}
void ChunkGenerator::Generate(const ChunkID id, Chunk &chunk) const
{
    int x0, x1, z0, z1, n, j;
    GLfloat h00, h10, h01, h11,
            htresh;
    vec2 p00, p01, p10, p11,
         p[4];
    TerrainIndex i = 0;

    chunk.terrainVertices.clear();
    chunk.terrainEdges.clear();

    for (htresh = 0.3f; htresh <= 0.6f; htresh += 0.15f)
    {
        for (x0 = 0; x0 < PER_CHUNK_SIZE; x0++)
        {
            x1 = x0 + 1;

            for (z0 = 0; z0 < PER_CHUNK_SIZE; z0++)
            {
                z1 = z0 + 1;

                p00 = Chunk2World(id, {x0, z0});
                p10 = Chunk2World(id, {x1, z0});
                p01 = Chunk2World(id, {x0, z1});
                p11 = Chunk2World(id, {x1, z1});

                h00 = GetHeight(mPerlin, p00);
                h10 = GetHeight(mPerlin, p10);
                h11 = GetHeight(mPerlin, p11);
                h01 = GetHeight(mPerlin, p01);

                n = 0;
                if (PointBetween(p00, h00, p01, h01, htresh, p[n]))
                    n++;
                if (PointBetween(p00, h00, p10, h10, htresh, p[n]))
                    n++;
                if (PointBetween(p10, h10, p11, h11, htresh, p[n]))
                    n++;
                if (PointBetween(p01, h01, p11, h11, htresh, p[n]))
                    n++;

                for (j = 0; j < n; j++)
                {
                    p[j] = World2Chunk(id, p[j]);

                    chunk.terrainVertices.push_back({vec3(p[j].x, 0.0f, p[j].z)});
                    i++;
                }

                if (n == 2)
                {
                    chunk.terrainEdges.push_back({i - 2, i - 1});
                }
                else if (n == 3)
                {
                    chunk.terrainEdges.push_back({i - 3, i - 1});
                    chunk.terrainEdges.push_back({i - 2, i - 1});
                    chunk.terrainEdges.push_back({i - 3, i - 2});
                }
                else if (n == 4)
                {
                    chunk.terrainEdges.push_back({i - 4, i - 1});
                    chunk.terrainEdges.push_back({i - 1, i - 2});
                    chunk.terrainEdges.push_back({i - 2, i - 3});
                    chunk.terrainEdges.push_back({i - 3, i - 4});
                }
            }
        }
    }
}
*/
template <int N>
GLfloat GetValue(const NoiseGenerator<N> *pNoise, const vec<N> &p)
{
    return pNoise->Noise(p / 50) + pNoise->Noise(p / 250);
}
/*GLfloat GetTreshold(const GLfloat y)
{
    return 0.001f * y * y;// * (0.85f + 0.15f * sin(y));
}
void ChunkGenerator::Generate(const ChunkID id, Chunk &chunk) const
{
    GLfloat y;
    int x0, x1, y0, y1, z0, z1,
        division = 2,  // the larger, the slower generation of chunks
        j, k, n, ei[4];

    UnitCubeVertexIndex vertexIndex;
    UnitCubeEdge edges[4];
    GLfloat a[N_CUBE_VERTICES];
    vec3 terrainCubeVertices[N_CUBE_VERTICES],
         cubeEdgeInterpolations[N_CUBE_EDGES];
    bool hasInterpolation[N_CUBE_EDGES];
    TerrainIndex i = 0,
                 terrainVertexIndicesPerEdge[N_CUBE_EDGES];

    chunk.terrainVertices.clear();
    chunk.terrainEdges.clear();

    // Iterate over cubes of size: PER_CHUNK_SIZE / division
    for (x0 = 0; x0 < PER_CHUNK_SIZE; x0 += PER_CHUNK_SIZE / division)
    {
        x1 = x0 + PER_CHUNK_SIZE / division;

        for (y0 = 0; y0 < MAX_WORLD_HEIGHT; y0 += PER_CHUNK_SIZE / 8)
        {
            y1 = y0 + PER_CHUNK_SIZE / 8;

            for (z0 = 0; z0 < PER_CHUNK_SIZE; z0 += PER_CHUNK_SIZE / division)
            {
                z1 = z0 + PER_CHUNK_SIZE / division;

                terrainCubeVertices[UNITCUBE_VERTEX_I000] = Chunk2World(id, {x0, y0, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I100] = Chunk2World(id, {x1, y0, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I001] = Chunk2World(id, {x0, y0, z1});
                terrainCubeVertices[UNITCUBE_VERTEX_I101] = Chunk2World(id, {x1, y0, z1});
                terrainCubeVertices[UNITCUBE_VERTEX_I010] = Chunk2World(id, {x0, y1, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I110] = Chunk2World(id, {x1, y1, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I011] = Chunk2World(id, {x0, y1, z1});
                terrainCubeVertices[UNITCUBE_VERTEX_I111] = Chunk2World(id, {x1, y1, z1});

                for (j = 0; j < N_CUBE_VERTICES; j++)
                    a[j] = GetValue(mPerlin, {terrainCubeVertices[j].x, terrainCubeVertices[j].z});

                for (j = 0; j < N_CUBE_EDGES; j++)
                {
                    UnitCubeEdge edge = unitCubeEdges[j];
                    UnitCubeVertexIndex i0 = edge.v[0],
                                        i1 = edge.v[1];

                    if (GetY(i0) && GetY(i1))
                        y = y1;
                    else if (!GetY(i0) && !GetY(i1))
                        y = y0;
                    else
                        y = 0.5f + y0;

                    if ((hasInterpolation[j] = PointBetween(terrainCubeVertices[i0], a[i0],
                                                            terrainCubeVertices[i1], a[i1], GetTreshold(y),
                                                            cubeEdgeInterpolations[j])))
                    {
                        cubeEdgeInterpolations[j] = World2Chunk(id, cubeEdgeInterpolations[j]);

                        chunk.terrainVertices.push_back({cubeEdgeInterpolations[j]});
                        terrainVertexIndicesPerEdge[j] = i;
                        i++;
                    }
                }

                for (j = 0; j < N_CUBE_EDGES; j++)
                {
                    for (k = 0; k < N_CUBE_EDGES; k++)
                    {
                        if (k != j &&
                            hasInterpolation[j] && hasInterpolation[k])
                        {
                            chunk.terrainEdges.push_back({terrainVertexIndicesPerEdge[j],
                                                          terrainVertexIndicesPerEdge[k]});
                        }
                    }
                }
            }
        }
    }
} */
void ChunkGenerator::Generate(const ChunkID id, Chunk &chunk) const
{
    int x0, x1, y0, y1, z0, z1,
        division = 2,  // the larger, the slower generation of chunks
        j, k, n, ei[4];

    UnitCubeVertexIndex vertexIndex;
    UnitCubeEdge edges[4];
    GLfloat a[N_CUBE_VERTICES],
            atresh = 0.0f;
    vec3 terrainCubeVertices[N_CUBE_VERTICES],
         cubeEdgeInterpolations[N_CUBE_EDGES];
    bool hasInterpolation[N_CUBE_EDGES];
    TerrainIndex i = 0,
                 terrainVertexIndicesPerEdge[N_CUBE_EDGES];

    chunk.terrainVertices.clear();
    chunk.terrainEdges.clear();

    // Iterate over cubes of size: PER_CHUNK_SIZE / division
    for (x0 = 0; x0 < PER_CHUNK_SIZE; x0 += PER_CHUNK_SIZE / division)
    {
        x1 = x0 + PER_CHUNK_SIZE / division;

        for (y0 = 0; y0 < MAX_WORLD_HEIGHT; y0 += PER_CHUNK_SIZE / division)
        {
            y1 = y0 + PER_CHUNK_SIZE / division;

            for (z0 = 0; z0 < PER_CHUNK_SIZE; z0 += PER_CHUNK_SIZE / division)
            {
                z1 = z0 + PER_CHUNK_SIZE / division;

                terrainCubeVertices[UNITCUBE_VERTEX_I000] = Chunk2World(id, {x0, y0, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I100] = Chunk2World(id, {x1, y0, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I001] = Chunk2World(id, {x0, y0, z1});
                terrainCubeVertices[UNITCUBE_VERTEX_I101] = Chunk2World(id, {x1, y0, z1});
                terrainCubeVertices[UNITCUBE_VERTEX_I010] = Chunk2World(id, {x0, y1, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I110] = Chunk2World(id, {x1, y1, z0});
                terrainCubeVertices[UNITCUBE_VERTEX_I011] = Chunk2World(id, {x0, y1, z1});
                terrainCubeVertices[UNITCUBE_VERTEX_I111] = Chunk2World(id, {x1, y1, z1});

                for (j = 0; j < N_CUBE_VERTICES; j++)
                    a[j] = GetValue(mNoiseGenerator, terrainCubeVertices[j]);

                for (j = 0; j < N_CUBE_EDGES; j++)
                {
                    UnitCubeEdge edge = unitCubeEdges[j];
                    UnitCubeVertexIndex i0 = edge.v[0],
                                        i1 = edge.v[1];

                    if ((hasInterpolation[j] = PointBetween(terrainCubeVertices[i0], a[i0],
                                                            terrainCubeVertices[i1], a[i1], atresh,
                                                            cubeEdgeInterpolations[j])))
                    {
                        cubeEdgeInterpolations[j] = World2Chunk(id, cubeEdgeInterpolations[j]);

                        chunk.terrainVertices.push_back({cubeEdgeInterpolations[j]});
                        terrainVertexIndicesPerEdge[j] = i;
                        i++;
                    }
                }

                for (j = 0; j < N_CUBE_EDGES; j++)
                {
                    for (k = 0; k < N_CUBE_EDGES; k++)
                    {
                        if (k != j &&
                            hasInterpolation[j] && hasInterpolation[k])
                        {
                            chunk.terrainEdges.push_back({terrainVertexIndicesPerEdge[j],
                                                          terrainVertexIndicesPerEdge[k]});
                        }
                    }
                }
            }
        }
    }
}
ChunkManager::ChunkManager(ChunkGenerator *pGen):
    pGenerator(pGen),
    bInitChunks(false),
    mChunkInitThread(NULL)
{
    mChunksMutex = SDL_CreateMutex();
    if (!mChunksMutex)
    {
        throw FormatableException("Error creating chunk manager mutex: %s", SDL_GetError());
    }
    mRequestsMutex = SDL_CreateMutex();
    if (!mRequestsMutex)
    {
        throw FormatableException("Error creating chunk manager mutex: %s", SDL_GetError());
    }
}
ChunkManager::~ChunkManager(void)
{
    StopChunkInitThread();

    SDL_DestroyMutex(mChunksMutex);
    SDL_DestroyMutex(mRequestsMutex);

    for (auto pair : mEntries)
        delete pair.second;
}
ChunkManager::ChunkEntry::ChunkEntry(void):
    mLastAccess(time(NULL)),
    mLastModify(time(NULL))
{
}
ChunkManager::ChunkEntry::~ChunkEntry(void)
{
}
void ChunkManager::DeleteChunk(const ChunkID &id)
{
    if (SDL_LockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    if (mEntries.find(id) != mEntries.end())
    {
        delete mEntries.at(id);
        mEntries.erase(id);
    }

    if (SDL_UnlockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }
}
void ChunkManager::DeleteChunksOlderThan(const double maxSeconds)
{
    if (SDL_LockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    time_t now = time(NULL);
    double seconds;

    std::list<ChunkID> toDelete;
    for (auto pair : mEntries)
    {
        seconds = difftime(now, pair.second->mLastAccess);
        if (seconds > maxSeconds)
        {
            toDelete.push_back(pair.first);
        }
    }
    for (const ChunkID &id : toDelete)
    {
        delete mEntries.at(id);
        mEntries.erase(id);
    }

    if (SDL_UnlockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }
}
void ChunkManager::GenerateChunk(const ChunkID &id)
{
    if (SDL_LockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    if (mEntries.find(id) == mEntries.end())
    {
        /*
            Temporarily unlock, to allow other threads to get
            chunks from the map while generating this one.
         */
        if (SDL_UnlockMutex(mChunksMutex) < 0)
        {
            throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
        }

        ChunkEntry *pEntry = new ChunkEntry;
        pGenerator->Generate(id, pEntry->mChunk);

        if (SDL_LockMutex(mChunksMutex) < 0)
        {
            throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
        }

        if (mEntries.find(id) == mEntries.end())
        {
            mEntries.emplace(id, pEntry);
            mEntries.at(id)->mLastModify = time(NULL);
        }
        else
        {   // not expected to happen if only one thread generates chunks
            delete pEntry;
        }
    }

    if (SDL_UnlockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }
}
bool ChunkManager::HasChunk(const ChunkID &id) const
{
    bool success = false;

    if (SDL_LockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    if (mEntries.find(id) != mEntries.end())
    {
        mEntries.at(id)->mLastAccess = time(NULL);
        success = true;
    }

    if (SDL_UnlockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }

    return success;
}
bool ChunkManager::GetChunk(const ChunkID &id, Chunk &chunk) const
{
    bool success = false;

    if (SDL_LockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    if (mEntries.find(id) != mEntries.end())
    {
        ChunkEntry *pEntry = mEntries.at(id);

        pEntry->mLastAccess = time(NULL);
        chunk = pEntry->mChunk;
        success = true;
    }

    if (SDL_UnlockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }

    return success;
}
bool ChunkManager::GetChunkModificationTime(const ChunkID &id, time_t &mtime) const
{
    bool success = false;

    if (SDL_LockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    if (mEntries.find(id) != mEntries.end())
    {
        ChunkEntry *pEntry = mEntries.at(id);

        pEntry->mLastAccess = time(NULL);
        mtime = pEntry->mLastModify;
        success = true;
    }

    if (SDL_UnlockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }

    return success;
}
bool ChunkManager::SetChunk(const ChunkID &id, const Chunk &chunk)
{
    bool success = false;

    if (SDL_LockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    if (mEntries.find(id) != mEntries.end())
    {
        ChunkEntry *pEntry = mEntries.at(id);

        pEntry->mLastModify = time(NULL);
        pEntry->mLastAccess = pEntry->mLastModify;
        pEntry->mChunk = chunk;
        success = true;
    }

    if (SDL_UnlockMutex(mChunksMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }

    return success;
}
void ChunkManager::TakeRequests(std::list<ChunkRequest> &dump)
{
    if (SDL_LockMutex(mRequestsMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    for (auto pair : mRequests)
        dump.push_back(pair.second);

    mRequests.clear();

    if (SDL_UnlockMutex(mRequestsMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }
}
// These are in seconds:
#define CHUNK_DELETE_INTERVAL 10.0f
#define MAX_CHUNK_IDLE_TIME 15.0f
void ChunkManager::StartChunkInitThread(ChunkHasPriorityOverFunc priorityFunc)
{
    bInitChunks = true;

    mChunkInitThread = MakeSDLThread(
        [this, priorityFunc]()
        {
            std::list<ChunkRequest> requests;
            Uint32 ticks0 = SDL_GetTicks(),
                   ticks;
            float delta;

            while (bInitChunks)
            {
                requests.clear();
                TakeRequests(requests);

                requests.sort(
                    [priorityFunc](const ChunkRequest &req1, const ChunkRequest &req2)
                    {
                        return priorityFunc(req1.id, req2.id);
                    }
                );

                for (const ChunkRequest &req : requests)
                {
                    if (!HasChunk(req.id))
                    {
                        GenerateChunk(req.id);
                    }

                    if (!bInitChunks)
                        break;
                }

                /*
                    Deleting old chunks is a costly operation,
                    iterating and locking the mutex.

                    So don't do every iteration.
                 */
                ticks = SDL_GetTicks();
                delta = float(ticks - ticks0) / 1000;

                if (delta > CHUNK_DELETE_INTERVAL)
                {
                    DeleteChunksOlderThan(MAX_CHUNK_IDLE_TIME);
                    ticks0 = ticks;
                }
            }

            return 0;
        },
    "init_chunks");
}
void ChunkManager::StopChunkInitThread(void)
{
    bInitChunks = false;

    int status;
    SDL_WaitThread(mChunkInitThread, &status);
}
void ChunkManager::RequestChunk(const ChunkID &id)
{
    if (SDL_LockMutex(mRequestsMutex) < 0)
    {
        throw FormatableException("Error locking chunk manager mutex: %s", SDL_GetError());
    }

    ChunkRequest req;
    req.id = id;
    mRequests[id] = req;

    if (SDL_UnlockMutex(mRequestsMutex) < 0)
    {
        throw FormatableException("Error unlocking chunk manager mutex: %s", SDL_GetError());
    }
}
void ChunkManager::RequestChunks(const vec2 &center, const GLfloat radius)
{
    ChunkCoord minCX = floor((center.x - radius) / PER_CHUNK_SIZE),
               minCZ = floor((center.z - radius) / PER_CHUNK_SIZE),
               maxCX = ceil((center.x + radius) / PER_CHUNK_SIZE),
               maxCZ = ceil((center.z + radius) / PER_CHUNK_SIZE),
               CX, CZ;

    for (CX = minCX; CX <= maxCX; CX ++)
    {
        for (CZ = minCZ; CZ <= maxCZ; CZ ++)
        {
            ChunkID id = MakeChunkID(CX, CZ);
            RequestChunk(id);
        }
    }
}
