#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <tuple>
#include <list>
#include <unordered_map>
#include <thread>

#include <glm/glm.hpp>
using namespace glm;

#include "concurrency.hpp"
#include "load.hpp"


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


/**
 *  Must be thread-safe!
 */
class ChunkWorker
{
    public:
        virtual void PrepareFor(const ChunkID) = 0;
        virtual void DestroyFor(const ChunkID) = 0;
        virtual float GetWorkRadius(void) const = 0;
};

/**
 *  Must be thread-safe!
 */
class ChunkObserver
{
    public:
        virtual vec3 GetWorldPosition(void) const = 0;
};

struct ChunkRecord
{
    bool updating,
         inRange;
    std::recursive_mutex mtxChunk;

    ChunkRecord(const ChunkRecord &);
    ChunkRecord(void);
};

struct ChunkWorkRecord
{
    ChunkWorker *pWorker;
    std::unordered_map<ChunkID, ChunkRecord> mChunks;

    ChunkWorkRecord(const ChunkWorkRecord &);
    ChunkWorkRecord(void);
};

class ChunkManager: public Initializable
{
    private:
        ConcurrentManager mChunkWorkManager;
        std::atomic<bool> working;
        static void ChunkWorkerThreadFunc(ChunkManager *);
        void FindOneJob(ChunkID &, ChunkWorkRecord *&);
        bool Updating(const ChunkID, const ChunkWorkRecord *);
        void GarbageCollect(void);

        std::list<std::exception_ptr> mErrors;
        std::recursive_mutex mtxError;
        void PushError(const std::exception_ptr &);

        std::recursive_mutex mtxLists;
        std::list<ChunkWorkRecord> mWorkRecords;
        std::list<const ChunkObserver *> observerPs;
    public:
        void Add(const ChunkObserver *);
        void Add(ChunkWorker *);

        void Start(void);
        void Stop(void);

        void ThrowAnyError(void);

        void TellInit(Loader &loader);  // Basically preloads some chunks.
        void DestroyAll(void);
};


#endif  // CHUNK_HPP
