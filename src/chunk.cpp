#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>

#include <boost/functional/hash.hpp>

#include "config.hpp"
#include "chunk.hpp"
#include "app.hpp"


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
std::tuple<float, float> GetChunkCenter(const ChunkID id)
{
    return std::make_tuple((float(id.x) * 0.5f) * CHUNK_SIZE,
                           (float(id.z) * 0.5f) * CHUNK_SIZE);
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

ChunkWorkRecord::ChunkWorkRecord(const ChunkWorkRecord &other)
 :pWorker(other.pWorker), mChunks(other.mChunks.begin(), other.mChunks.end())
{
}
ChunkWorkRecord::ChunkWorkRecord(void)
 :pWorker(NULL)
{
}
ChunkRecord::ChunkRecord(const ChunkRecord &other)
 :updating(other.updating), inRange(other.inRange)
{
}
ChunkRecord::ChunkRecord(void)
 :updating(false), inRange(false)
{
}
ChunkManager::ChunkManager(const WorldSeed seed)
: mSeed(seed)
{
}
ChunkManager::~ChunkManager(void)
{
    Stop();
}
void ChunkManager::Start(void)
{
    working = true;

    Config config;
    App::Instance().GetConfig(config);

    mChunkWorkManager.Start(max(1, int(config.loadConcurrency - 1)), ChunkWorkerThreadFunc, this);
    mGarbageCollectThread = std::thread(ChunkGarbageCollectThreadFunc, this);
}
void ChunkManager::Stop(void)
{
    working = false;

    mChunkWorkManager.JoinAll();

    if (mGarbageCollectThread.joinable())
        mGarbageCollectThread.join();

    ThrowAnyError();
}
void ChunkManager::ChunkWorkerThreadFunc(ChunkManager *p)
{
    ChunkWorkRecord *pRecord;
    ChunkID id;

    while (p->working)
    {
        if (!(p->FindOneJob(id, pRecord)))
            continue;

        try
        {
            pRecord->pWorker->PrepareFor(id, p->mSeed);
        }
        catch (...)
        {
            p->PushError(std::current_exception());
        }
    }
}
void ChunkManager::ChunkGarbageCollectThreadFunc(ChunkManager *p)
{
    while (p->working)
    {
        try
        {
            p->GarbageCollect();
        }
        catch (...)
        {
            p->PushError(std::current_exception());
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
void ChunkManager::Add(ChunkWorker *p)
{
    std::scoped_lock(mtxLists);

    ChunkWorkRecord record;
    record.pWorker = p;

    mWorkRecords.push_back(record);
}
void ChunkManager::Add(const ChunkObserver *p)
{
    std::scoped_lock(mtxLists);

    observerPs.push_back(p);
}
void ChunkManager::PushError(const std::exception_ptr &e)
{
    std::scoped_lock(mtxErrors);
    mErrors.push_back(e);
}
void ChunkManager::ThrowAnyError(void)
{
    std::scoped_lock(mtxErrors);

    if (mErrors.size() > 0)
        std::rethrow_exception(mErrors.front());
}
void ChunkManager::DestroyAll(void)
{
    std::scoped_lock(mtxLists);

    for (ChunkWorkRecord &record : mWorkRecords)
    {
        for (auto &pair : record.mChunks)
        {
            record.pWorker->DestroyFor(pair.first);
        }

        record.mChunks.clear();
    }
}
void ChunkManager::GarbageCollect(void)
{
    std::scoped_lock lock(mtxLists);

    float radius, cx, cz, dx, dz;
    vec3 pos;

    for (ChunkWorkRecord &record : mWorkRecords)
    {
        radius = record.pWorker->GetWorkRadius();

        for (auto &pair : record.mChunks)
            pair.second.inRange = false;

        for (const ChunkObserver *pObserver : observerPs)
        {
            pos = pObserver->GetWorldPosition();

            for (auto &pair : record.mChunks)
            {
                std::tie(cx, cz) = GetChunkCenter(pair.first);

                dx = cx - pos.x;
                dz = cz - pos.z;

                if ((dx * dx + dz * dz) < radius * radius)
                {
                    pair.second.inRange = true;
                }
            }
        }

        auto it = record.mChunks.begin();
        while (it != record.mChunks.end())
        {
            if (!(it->second.inRange))
            {
                record.pWorker->DestroyFor(it->first);

                it = record.mChunks.erase(it);
            }
            else
                it++;
        }
    }
}
class ChunkPrepareJob: public LoadJob
{
    private:
        ChunkID id;
        WorldSeed seed;
        ChunkWorkRecord *pRecord;
    public:
        ChunkPrepareJob(ChunkWorkRecord *p, const ChunkID cid, const WorldSeed s)
         :pRecord(p), id(cid), seed(s)
        {
        }

        void Run(void)
        {
            pRecord->pWorker->PrepareFor(id, seed);
            pRecord->mChunks[id].updating = true;
        }
};
void ChunkManager::TellInit(Loader &loader)
{
    float radius, x, z;
    vec3 pos;

    for (ChunkWorkRecord &record : mWorkRecords)
    {
        radius = record.pWorker->GetWorkRadius();

        for (const ChunkObserver *pObserver : observerPs)
        {
            pos = pObserver->GetWorldPosition();

            for (x = pos.x - radius; x < (pos.x + radius); x += CHUNK_SIZE)
            {
                for (z = pos.z - radius; z < (pos.z + radius); z += CHUNK_SIZE)
                {
                    loader.Add(new ChunkPrepareJob(&record, GetChunkID(x, z), mSeed));
                }
            }
        }
    }
}
bool ChunkManager::FindOneJob(ChunkID &id, ChunkWorkRecord *&pRecord)
{
    float x, z, dx, dz, radius;
    vec3 pos;

    int64_t r;
    int64_t chx, chz;

    ChunkID centerID;

    std::scoped_lock lock(mtxLists);

    for (ChunkWorkRecord &record : mWorkRecords)
    {
        radius = record.pWorker->GetWorkRadius();

        for (const ChunkObserver *pObserver : observerPs)
        {
            pos = pObserver->GetWorldPosition();

            centerID = GetChunkID(pos.x, pos.z);

            /* Fill in nearby chunks, closest get priority.
               After updating a single chunk, immediatly return so
               that the center position can be reset.
             */
            for (r = 0; (r * CHUNK_SIZE) < radius; r++)
            {
                for (chx = -r; chx <= r; chx++)
                {
                    id.x = centerID.x + chx;

                    id.z = centerID.z + r;
                    if (!Updating(id, &record))
                    {
                        pRecord = &record;
                        pRecord->mChunks[id].updating = true;
                        return true;
                    }
                    id.z = centerID.z - r;
                    if (!Updating(id, &record))
                    {
                        pRecord = &record;
                        pRecord->mChunks[id].updating = true;
                        return true;
                    }
                }
                for (chz = -r; chz <= r; chz++)
                {
                    id.z = centerID.z + chz;

                    id.x = centerID.x + r;
                    if (!Updating(id, &record))
                    {
                        pRecord = &record;
                        pRecord->mChunks[id].updating = true;
                        return true;
                    }
                    id.x = centerID.x - r;
                    if (!Updating(id, &record))
                    {
                        pRecord = &record;
                        pRecord->mChunks[id].updating = true;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}
bool ChunkManager::Updating(const ChunkID id, const ChunkWorkRecord *pRecord)
{
    if (pRecord->mChunks.find(id) == pRecord->mChunks.end())
        return false;

    return pRecord->mChunks.at(id).updating;
}
