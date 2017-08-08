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

#include <time.h>
#include <math.h>
#include <algorithm>

#include "terrain.h"
#include "play.h"
#include "matrix.h"
#include "exception.h"
#include "log.h"


PlayScene::PlayScene(Client *pCl):
    Scene(pCl),
    yaw(0.0f), pitch(0.0f), pos(0.0f, 1.8f, 0.0f),
    mCursor(NULL),
    mWorldRenderer(NULL)
{
    mMenuScreenOverlay = new TransparentBlueScreenFader(pClient);

    mInitJobs.push_back([this](){
        mCursor = new MenuCursor(pClient);
    });

    mInitJobs.push_back([this](){
        InitMenus();
    });

    mInitJobs.push_back([this](){
        mWorldRenderer = new WorldRenderer(pClient);
    });

    ReportPositionToServer();
}
PlayScene::~PlayScene(void)
{
    for (auto pair : mCachedRenderChunks)
        delete pair.second;

    for (auto pair : mMenus)
        delete pair.second;

    delete mCursor;
    delete mMenuScreenOverlay;

    delete mWorldRenderer;
}
#define MENUID_ESCAPE 1
#define MENUID_OPTIONS 2
#define MENUID_VIDEO 3
#define MENUID_CONTROLS 4
#define MENUID_LANGUAGE 5
#define MENUID_QUIT 6
void PlayScene::InitMenus(void)
{
    mMenus.emplace(MENUID_ESCAPE, new Menu());
    mMenus.emplace(MENUID_OPTIONS, new Menu());
    mMenus.emplace(MENUID_VIDEO, new Menu());
    mMenus.emplace(MENUID_CONTROLS, new Menu());
    mMenus.emplace(MENUID_LANGUAGE, new Menu());
    mMenus.emplace(MENUID_QUIT, new Menu());

    InitYesNoMenu(mMenus[MENUID_QUIT],
                  std::make_shared<LanguageStringProvider>(pClient, "menu.question.quit"),
                  [this]()
                  {
                      pClient->ShutDown();
                  },
                  [this]()
                  {
                      UnescapeFromMenu();
                  },
                  pClient);
    InitEscapeMenu(mMenus[MENUID_ESCAPE], mMenus[MENUID_OPTIONS], this, pClient);
    InitOptionsMenu(mMenus[MENUID_OPTIONS], mMenus[MENUID_VIDEO],
                    mMenus[MENUID_CONTROLS], mMenus[MENUID_LANGUAGE],
                    mMenus[MENUID_ESCAPE], pClient);
    InitVideoMenu(mMenus[MENUID_VIDEO], mMenus[MENUID_OPTIONS], pClient);
    InitControlsMenu(mMenus[MENUID_CONTROLS], mMenus[MENUID_OPTIONS], pClient);
    InitLanguageMenu(mMenus[MENUID_LANGUAGE], mMenus[MENUID_OPTIONS], pClient);
}
void PlayScene::ReportPositionToServer(void)
{
    DataPackage pkg;
    PositionUpdate u;

    u.pos = pos;
    pkg.type = DATA_POSITION_UPDATE;
    pkg.data = (void *)&u;
    pkg.size = sizeof(u);

    pClient->GetServerInterface()->SendData(pkg);
}
PlayScene::ChunkRequestData::ChunkRequestData(void):
    lastRequestTime(0)
{
}
void PlayScene::RequestChunkFromServer(const ChunkID &id)
{
    DataPackage pkg;
    ChunkRequest req;

    req.id = id;
    pkg.type = DATA_CHUNK_REQUEST;
    pkg.data = (void *)&req;
    pkg.size = sizeof(req);

    pClient->GetServerInterface()->SendData(pkg);
}
#define MOVEMENT_SPEED 50.0f
#define CHUNK_REQUEST_INTERVAL 5.0
void PlayScene::Update(const float dt)
{
    ClientSettings settings;
    pClient->GetSettings(settings);

    if (pClient->KeyIsDown(KEYB_GOFORWARD))
        pos += MOVEMENT_SPEED * dt * (MatRotY(yaw) * vec3(0.0f, 0.0f, -1.0f));
    else if (pClient->KeyIsDown(KEYB_GOBACK))
        pos += MOVEMENT_SPEED * dt * (MatRotY(yaw) * vec3(0.0f, 0.0f, 1.0f));

    if (pClient->KeyIsDown(KEYB_GOLEFT))
        pos += MOVEMENT_SPEED * dt * (MatRotY(yaw) * vec3(-1.0f, 0.0f, 0.0f));
    else if (pClient->KeyIsDown(KEYB_GORIGHT))
        pos += MOVEMENT_SPEED * dt * (MatRotY(yaw) * vec3(1.0f, 0.0f, 0.0f));


    if (pClient->KeyIsDown(KEYB_JUMP))
        pos += MOVEMENT_SPEED * dt * vec3(0.0f, 1.0f, 0.0f);
    else if (pClient->KeyIsDown(KEYB_DUCK))
        pos += MOVEMENT_SPEED * dt * vec3(0.0f, -1.0f, 0.0f);

    // The server should be able to handle one player update per frame.
    ReportPositionToServer();

    /*
        Request any chunks that might be missing:
     */
    ChunkCoord minCX = floor((pos.x - settings.render.renderDistance) / PER_CHUNK_SIZE),
               minCZ = floor((pos.z - settings.render.renderDistance) / PER_CHUNK_SIZE),
               maxCX = ceil((pos.x + settings.render.renderDistance) / PER_CHUNK_SIZE),
               maxCZ = ceil((pos.z + settings.render.renderDistance) / PER_CHUNK_SIZE),
               CX, CZ;

    time_t now = time(NULL);
    for (CX = minCX; CX <= maxCX; CX++)
    {
        for (CZ = minCZ; CZ <= maxCZ; CZ++)
        {
            ChunkID chunkID = MakeChunkID(CX, CZ);
            if (mCachedRenderChunks.find(chunkID) == mCachedRenderChunks.end())
            {
                if (mChunkRequests.find(chunkID) == mChunkRequests.end())
                    mChunkRequests.emplace(chunkID, ChunkRequestData());

                /*
                    Don't spam the server with chunk requests. Only send the same
                    request once in a while.
                 */
                if (difftime(now, mChunkRequests.at(chunkID).lastRequestTime) > CHUNK_REQUEST_INTERVAL)
                {
                    RequestChunkFromServer(chunkID);

                    mChunkRequests.at(chunkID).lastRequestTime = now;
                }
            }
        }
    }

    /*
        Update the gui:
     */
    mCursor->Update(dt);
    mMenuScreenOverlay->Update(dt);

    for (auto pair : mMenus)
        pair.second->Update(dt);
}
void glColorHSV(float h, float s, float v)
{
     // H [0, 360] S and V [0.0, 1.0].
     int i = (int)floor(h/60.0f) % 6;
     float f = h / 60.0f - floor(h / 60.0f);
     float p = v * (float)(1 - s);
     float q = v * (float)(1 - s * f);
     float t = v * (float)(1 - (1 - f) * s);

     switch (i) {
         case 0: glColor3f(v, t, p);
         break;
         case 1: glColor3f(q, v, p);
         break;
         case 2: glColor3f(p, v, t);
         break;
         case 3: glColor3f(p, q, v);
         break;
         case 4: glColor3f(t, p, v);
         break;
         case 5: glColor3f(v, p, q);
    }
}
void PlayScene::Render(void)
{
    FirstPersonCamera camera(pos, pitch, yaw);
    mWorldRenderer->RenderAll(mCachedRenderChunks, &camera);

    mMenuScreenOverlay->Render();

    if (AnyMenuVisible())
    {
        RenderMenuOverlay(pClient, mCursor, mMenus);
    }
}
bool PlayScene::OnChunkUpdate(const ChunkUpdate *p)
{
    mCachedRenderChunks[p->id] = new ChunkRenderObject(p->id, p->chunk);

    return false;
}
bool PlayScene::OnEvent(const SDL_Event &event)
{
    if (EventProcessor::OnEvent(event))
        return true;

    for (auto pair : mMenus)
    {
        if (pair.second->OnEvent(event))
            return true;
    }
    return false;
}
#define MOUSEMOVE_SENSITIVITY 0.007f
bool PlayScene::OnMouseMotion(const SDL_MouseMotionEvent &event)
{
    if (!AnyMenuVisible())
    {
        yaw -= MOUSEMOVE_SENSITIVITY * event.xrel;
        pitch -= MOUSEMOVE_SENSITIVITY * event.yrel;

        // clamp this angle:
        if (pitch < -1.45f)
            pitch = -1.45f;
        if (pitch > 1.45f)
            pitch = 1.45f;

        if (!SDL_GetRelativeMouseMode())
        {
            if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0)
            {
                LOG_ERROR("cannot set relative mouse mode: %s",
                          SDL_GetError());
            }
            pitch = 0.0f;
            yaw = 0.0f;
        }
    }
    return false;
}
bool PlayScene::AnyMenuVisible(void)
{
    for (auto pair : mMenus)
    {
        if (pair.second->IsActive() || !(pair.second->Ready()))
            return true;
    }

    return false;
}
void PlayScene::EscapeToMenu(Menu *pMenu)
{
    if (SDL_SetRelativeMouseMode(SDL_FALSE) < 0)
    {
        LOG_ERROR("cannot set relative mouse mode: %s",
                  SDL_GetError());
    }

    mMenuScreenOverlay->StartFadeIn();

    if (!AnyMenuVisible())
    {
        ClientSettings settings;
        pClient->GetSettings(settings);

        SDL_WarpMouseInWindow(NULL, settings.display.resolution.width / 2,
                                    settings.display.resolution.height / 2);
    }

    pMenu->Enter();
}
void PlayScene::UnescapeFromMenu(void)
{
    for (auto pair : mMenus)
        if (pair.second->IsActive())
            pair.second->Leave(
                [this]()
                {
                    mMenuScreenOverlay->StartFadeOut();
                }
            );

    if (SDL_SetRelativeMouseMode(SDL_TRUE) < 0)
    {
        LOG_ERROR("cannot set relative mouse mode: %s",
                  SDL_GetError());
    }
}
bool PlayScene::OnBoundKeyDown(const KeyBinding keyb)
{
    if (keyb == KEYB_MENU)
    {
        if (AnyMenuVisible())
        {
            UnescapeFromMenu();
        }
        else
        {
            EscapeToMenu(mMenus[MENUID_ESCAPE]);
        }
        return true;
    }
    return false;
}
bool PlayScene::OnQuit(const SDL_QuitEvent &)
{
    Menu *pActiveMenu = NULL;
    for (auto pair : mMenus)
        if (pair.second->IsActive())
            pActiveMenu = pair.second;

    if (pActiveMenu)
    {
        pActiveMenu->Leave([this](){
            EscapeToMenu(mMenus[MENUID_QUIT]);
        });
    }
    else
    {
        EscapeToMenu(mMenus[MENUID_QUIT]);
    }

    return true;
}
