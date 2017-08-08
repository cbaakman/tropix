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

#ifndef PLAY_H
#define PLAY_H

#include <time.h>
#include <vector>
#include <unordered_map>

#include "voronoi.h"
#include "terrain.h"
#include "vec.h"
#include "client.h"
#include "menu.h"
#include "render.h"


class PlayScene : public Scene
{
private:
    vec3 pos;
    GLfloat pitch, yaw;

    WorldRenderer *mWorldRenderer;

    struct ChunkRequestData
    {
        time_t lastRequestTime;

        ChunkRequestData(void);
    };

    ChunkRenderCache mCachedRenderChunks;
    std::unordered_map<ChunkID, ChunkRequestData> mChunkRequests;

    MenuCursor *mCursor;
    MenuMap mMenus;

    ScreenFader *mMenuScreenOverlay;
private:
    void InitMenus(void);

    bool AnyMenuVisible(void);

    void ReportPositionToServer(void);
    void RequestChunkFromServer(const ChunkID &);
public:
    PlayScene(Client *);
    ~PlayScene(void);

    void Update(const float dt);
    void Render(void);

    void EscapeToMenu(Menu *);
    void UnescapeFromMenu(void);
protected:
    bool OnEvent(const SDL_Event &);

    bool OnMouseMotion(const SDL_MouseMotionEvent &);
    bool OnBoundKeyDown(const KeyBinding);
    bool OnQuit(const SDL_QuitEvent &);

    bool OnChunkUpdate(const ChunkUpdate *);
};

#endif  // PLAY_H
