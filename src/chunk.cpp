#include <cmath>

#include <boost/functional/hash.hpp>

#include "chunk.hpp"


bool ChunkID::operator==(const ChunkID &other) const
{
    return x == other.x && z == other.z;
}
bool ChunkID::operator!=(const ChunkID &other) const
{
    return x != other.x || z != other.z;
}

ChunkID GetChunkID(const float x, const float z)
{
    ChunkID id;

    id.x = (int64_t)floor(x / CHUNK_SIZE);
    id.z = (int64_t)floor(z / CHUNK_SIZE);

    return id;
}

namespace std
{
    size_t hash<ChunkID>::operator()(const ChunkID &id) const
    {
        size_t h = 0;
        size_t hx = hash<int64_t>{}(id.x);
        size_t hz = hash<int64_t>{}(id.z);

        boost::hash_combine(h, hx);
        boost::hash_combine(h, hz);
        return h;
    }
}
