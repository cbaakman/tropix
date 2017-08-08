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

#ifndef RENDER_H
#define RENDER_H

#include <memory>

#include "matrix.h"
#include "provider.h"
#include "chunk.h"
#include "client.h"


enum RenderPhase3D
{
    RENDER_OPAQUE,
    RENDER_TRANSPARENT,
    RENDER_FROM_ALL  // like reflective or refractive
};

class ChunkRenderObject
{
private:
    GLuint mDisplayList;
public:
    void Load(const ChunkID &, const Chunk &);

    void Render(void) const;

    ChunkRenderObject(void);
    ChunkRenderObject(const ChunkID &, const Chunk &);
    ~ChunkRenderObject(void);
};


class Camera
{
public:
    virtual void GetViewMatrix(matrix4 &) const = 0;
    virtual void GetPosition(vec3 &) const = 0;
};

class FirstPersonCamera : public Camera
{
private:
    vec3 mPos;
    GLfloat mPitch,
            mYaw;
public:
    void SetFirstPerson(const vec3 &pos,
                        const GLfloat pitch,
                        const GLfloat yaw);

    FirstPersonCamera(const vec3 &pos,
                      const GLfloat pitch,
                      const GLfloat yaw);

    void GetViewMatrix(matrix4 &) const;
    void GetPosition(vec3 &) const;
};

typedef std::unordered_map<ChunkID, ChunkRenderObject *> ChunkRenderCache;

class WorldRenderer
{
private:
    Client *pClient;

    GLuint fogShaderProgram;
private:
    void RenderTerrain(const ChunkRenderCache &,
                       const Camera *, const float renderDistance);
public:
    WorldRenderer(Client *);
    ~WorldRenderer(void);

    void RenderAll(const ChunkRenderCache &, const Camera *);
};

#endif  // RENDER_H
