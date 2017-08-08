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

#include <stdio.h>
#include <cstring>

#include <SDL2/SDL.h>

#include "server.h"
#include "exception.h"
#include "log.h"


Server::PlayerChunkData::PlayerChunkData(void):
    lastUpdate(time(NULL)),
    requested(false)
{
}
Server::Server(void):
    done(false),
    mChunkRadius(500.0f)
{
    mChunkGenerator = new ChunkGenerator(time(NULL));
    mChunkManager = new ChunkManager(mChunkGenerator);

    mChunkManager->StartChunkInitThread(
        [this](const ChunkID &id1, const ChunkID &id2)
        {
            return GetChunkPriority(id1) < GetChunkPriority(id2);
        }
    );
}
Server::~Server(void)
{
    delete mChunkManager;
    delete mChunkGenerator;
}
bool Server::Run()
{
    ClientID clientID;
    DataPackage pkg;
    const char *error;
    Uint32 lastTicks, ticks;
    float dt;

    done = false;
    while (!done)
    {
        try
        {
            while (PollFromClient(clientID, pkg))
            {
                HandleClientData(clientID, pkg);
                free(pkg.data);
            }

            ticks = SDL_GetTicks();
            dt = float(ticks - lastTicks) / 1000;
            lastTicks = ticks;

            Update(dt);
        }
        catch (std::exception &e)
        {
            DisplayError(e.what());
        }
    }

    return true;
}
float Server::GetChunkPriority(const ChunkID &chunkID)
{
    vec2 chunkCenter = Chunk2World(chunkID, {GLfloat(PER_CHUNK_SIZE) / 2,
                                             GLfloat(PER_CHUNK_SIZE) / 2});

    float minDist2 = -1.0f;

    for (auto &pair : mPlayerData)
    {
        vec2 playerPos = {pair.second.pos.x, pair.second.pos.z};
        if (minDist2 < 0.0f || Distance2(playerPos, chunkCenter) < minDist2)
            minDist2 = Distance2(playerPos, chunkCenter);
    }

    // Closest chunks go first.
    return 1.0f / minDist2;
}
void Server::Update(const float dt)
{
    for (auto &pair : mPlayerData)
    {
        vec3 center = pair.second.pos;
        ChunkCoord minCX = floor((center.x - mChunkRadius) / PER_CHUNK_SIZE),
                   minCZ = floor((center.z - mChunkRadius) / PER_CHUNK_SIZE),
                   maxCX = ceil((center.x + mChunkRadius) / PER_CHUNK_SIZE),
                   maxCZ = ceil((center.z + mChunkRadius) / PER_CHUNK_SIZE),
                   CX = 0,
                   CZ = 0;

        for (CX = minCX; CX <= maxCX; CX ++)
        {
            for (CZ = minCZ; CZ <= maxCZ; CZ ++)
            {
                ChunkID id = MakeChunkID(CX, CZ);
                time_t mtime;

                if (pair.second.chunkData.find(id) == pair.second.chunkData.end() ||
                        pair.second.chunkData.at(id).requested ||
                        mChunkManager->GetChunkModificationTime(id, mtime) &&
                        mtime > pair.second.chunkData.at(id).lastUpdate)
                {
                    /*
                        Must limit the number of chunk updates,
                        not to spam the client.
                        Only send each chunk once, unless modified
                        or requested.
                     */

                    Chunk chunk;
                    if (mChunkManager->GetChunk(id, chunk))
                    {
                        SendChunkToClient(pair.first, id, chunk);

                        if (pair.second.chunkData.find(id) == pair.second.chunkData.end())
                            pair.second.chunkData.emplace(id, PlayerChunkData());

                        pair.second.chunkData.at(id).requested = false;
                        pair.second.chunkData.at(id).lastUpdate = mtime;
                    }
                }
            }
        }
    }
}
void Server::SendChunkToClient(const ClientID &clientID, const ChunkID &chunkID, const Chunk &chunk)
{
    DataPackage pkg;

    void *pChunkData;
    size_t szChunkData;
    std::tie(pChunkData, szChunkData) = Serialize(chunk);

    pkg.type = DATA_CHUNK_UPDATE;
    pkg.size = sizeof(ChunkID) + szChunkData;
    pkg.data = malloc(pkg.size);
    memcpy(pkg.data, &chunkID, sizeof(chunkID));
    memcpy(pkg.data + sizeof(chunkID), pChunkData, szChunkData);
    free(pChunkData);

    SendToClient(clientID, pkg);

    free(pkg.data);
}
void Server::OnPlayerPositionUpdate(const ClientID &clientID, const PositionUpdate *p)
{
    if (mPlayerData.find(clientID) == mPlayerData.end())
    {
        mPlayerData.emplace(clientID, PlayerData());
    }

    mPlayerData.at(clientID).pos = p->pos;
    mChunkManager->RequestChunks({p->pos.x, p->pos.z}, mChunkRadius);
}
void Server::OnPlayerChunkRequest(const ClientID &clientID, const ChunkRequest *p)
{
    if (mPlayerData.find(clientID) == mPlayerData.end())
    {
        mPlayerData.emplace(clientID, PlayerData());
    }

    if (mPlayerData.at(clientID).chunkData.find(p->id) == mPlayerData.at(clientID).chunkData.end())
    {
        mPlayerData.at(clientID).chunkData.emplace(p->id, PlayerChunkData());
    }

    mPlayerData.at(clientID).chunkData.at(p->id).requested = true;
}
void Server::HandleClientData(const ClientID &clientID, const DataPackage &pkg)
{
    switch (pkg.type)
    {
    case DATA_PING:
    {
        PingRequest *req = (PingRequest *)pkg.data;

        PingRequest res;
        res.bounces = req->bounces - 1;

        if (req->bounces > 0)
        {
            DataPackage pkgRes;
            pkgRes.type = DATA_PING;
            pkgRes.data = (void *)&res;
            pkgRes.size = sizeof(res);
            SendToClient(clientID, pkgRes);
        }
        break;
    }
    case DATA_CHUNK_REQUEST:
        OnPlayerChunkRequest(clientID, (ChunkRequest *)pkg.data);
        break;
    case DATA_POSITION_UPDATE:
        OnPlayerPositionUpdate(clientID, (PositionUpdate *)pkg.data);
        break;
    }
}
void Server::DisplayError(const std::string &error)
{
    fprintf(stderr, "server error: %s\n", error.c_str());
}
void Server::ShutDown(void)
{
    done = true;
}
InternalServer::InternalServer(void)
{
    mToClientMutex = SDL_CreateMutex();
    if (mToClientMutex == NULL)
        throw FormatableException("cannot create internal server mutex: %s", SDL_GetError());

    mFromClientMutex = SDL_CreateMutex();
    if (mFromClientMutex == NULL)
        throw FormatableException("cannot create internal server mutex: %s", SDL_GetError());
}
InternalServer::~InternalServer(void)
{
    SDL_DestroyMutex(mToClientMutex);
    SDL_DestroyMutex(mFromClientMutex);
}
bool InternalServer::PollFromClient(ClientID &clientID, DataPackage &pkg)
{
    bool result;

    if (SDL_LockMutex(mFromClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot lock fromclient mutex: %s", SDL_GetError());
    }

    if (mDataFromClient.empty())
        result = false;
    else
    {
        // There's only one client in an internal server:
        clientID = 0;
        memcpy(&pkg, &mDataFromClient.front(), sizeof(pkg));
        mDataFromClient.pop();
        result = true;
    }

    if (SDL_UnlockMutex(mFromClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot unlock client message mutex: %s", SDL_GetError());
    }

    return result;
}
void InternalServer::SendToClient(const ClientID &id, const DataPackage &pkg)
{
    if (SDL_LockMutex(mToClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot lock toclient mutex: %s", SDL_GetError());
    }

    DataPackage sendPkg = pkg;
    sendPkg.data = malloc(pkg.size);
    memcpy(sendPkg.data, pkg.data, pkg.size);

    // There's only one client in an internal server:
    mDataToClient.push(sendPkg);

    if (SDL_UnlockMutex(mToClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot unlock toclient mutex: %s", SDL_GetError());
    }
}
void InternalServer::PushDataFromClient(const DataPackage &pkg)
{
    if (SDL_LockMutex(mFromClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot lock fromclient mutex: %s", SDL_GetError());
    }

    DataPackage storePkg = pkg;
    storePkg.data = malloc(pkg.size);
    memcpy(storePkg.data, pkg.data, pkg.size);

    // There's only one client in an internal server:
    mDataFromClient.push(storePkg);

    if (SDL_UnlockMutex(mFromClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot unlock fromclient mutex: %s", SDL_GetError());
    }
}
bool InternalServer::PopDataFromServer(DataPackage &pkg)
{
    // Not asking the client's identity, there is only one ;)

    bool result;

    if (SDL_LockMutex(mToClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot lock toclient mutex: %s", SDL_GetError());
    }

    if (mDataToClient.empty())
    {
        result = false;
    }
    else
    {
        memcpy(&pkg, &mDataToClient.front(), sizeof(pkg));
        mDataToClient.pop();
        result = true;
    }

    if (SDL_UnlockMutex(mToClientMutex) < 0)
    {
        throw FormatableException(
                   "Internal server cannot unlock toclient mutex: %s", SDL_GetError());
    }

    return result;
}
