#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <functional>
#include <tuple>


#define TILE_SIZE 1.0f
#define COUNT_CHUNKROW_TILES 100

#define CHUNK_SIZE (TILE_SIZE * COUNT_CHUNKROW_TILES)


struct ChunkID
{
    int64_t x, z;

    bool operator==(const ChunkID &) const;
    bool operator!=(const ChunkID &) const;
};

namespace std
{
    template <>
    struct hash<ChunkID>
    {
        size_t operator()(const ChunkID &) const;
    };
}

ChunkID GetChunkID(const float x, const float z);
std::tuple<float, float> GetChunkCenter(const ChunkID id);

#endif  // CHUNK_HPP
