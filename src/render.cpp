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

#include "render.h"
#include "log.h"
#include "terrain.h"
#include "shader.h"
#include "exception.h"

const char fogVSH[] = R"shader(
    #version 150

    uniform vec3 fogColor;

    uniform mat4 projMatrix,
                 modelViewMatrix;

    uniform vec3 cameraPos;

    uniform float maxRenderDist;

    in vec3 vertex;

    out VertexData {
        vec3 color;
    } VertexOut;

    void main ()
    {
        float deep = max(0.0, -vertex.y),
              blueness = min(1.0, deep / 10),
              dist2 = pow(length(cameraPos - vertex), 2),
              fog = min(1.0, dist2 / pow(maxRenderDist, 2)),
              nonfog = 1.0 - fog;

        gl_Position = projMatrix * modelViewMatrix * vec4(vertex, 1.0);

        VertexOut.color.r = fog * fogColor.r + nonfog * (1.0 - blueness);
        VertexOut.color.g = fog * fogColor.g + nonfog * (1.0 - blueness);
        VertexOut.color.b = fog * fogColor.b + nonfog * (0.5 + 0.5 * blueness);
    }
)shader",

          fogFSH[] = R"shader(
    #version 150

    in VertexData {
        vec3 color;
    } VertexIn;

    out vec4 FragColor;

    void main ()
    {
        FragColor = vec4(VertexIn.color, 1.0);
    }
)shader";


void ChunkRenderObject::Load(const ChunkID &id, const Chunk &chunk)
{
    vec3 p1, p2;

    glNewList(mDisplayList, GL_COMPILE);

    /* glBegin(GL_POINTS);
    for (const TerrainVertex &v : chunk.terrainVertices)
    {
        p1 = Chunk2World(id, v.pos);

        glVertex3f(p1.x, p1.y, p1.z);
    }
    glEnd(); */

    glBegin(GL_LINES);
    for (const TerrainEdge &edge : chunk.terrainEdges)
    {
        p1 = Chunk2World(id, chunk.terrainVertices[edge.i1].pos);
        p2 = Chunk2World(id, chunk.terrainVertices[edge.i2].pos);

        glVertex3f(p1.x, p1.y, p1.z);
        glVertex3f(p2.x, p2.y, p2.z);
    }
    glEnd();

    glEndList();
}
void ChunkRenderObject::Render(void) const
{
    glCallList(mDisplayList);
}
ChunkRenderObject::ChunkRenderObject(void)
{
    mDisplayList = glGenLists(1);
}
ChunkRenderObject::ChunkRenderObject(const ChunkID &id, const Chunk &chunk):
    ChunkRenderObject()
{
    Load(id, chunk);
}
ChunkRenderObject::~ChunkRenderObject(void)
{
    glDeleteLists(mDisplayList, 1);
}
FirstPersonCamera::FirstPersonCamera(const vec3 &pos,
                                     const GLfloat pitch,
                                     const GLfloat yaw):
    mPos(pos),
    mPitch(pitch),
    mYaw(yaw)
{
}
void FirstPersonCamera::SetFirstPerson(const vec3 &pos,
                                       const GLfloat pitch,
                                       const GLfloat yaw)
{
    mPos = pos;
    mPitch = pitch;
    mYaw = yaw;
}
void FirstPersonCamera::GetViewMatrix(matrix4 &m) const
{
    m = MatFirstPerson(mPos, mYaw, mPitch);
}
void FirstPersonCamera::GetPosition(vec3 &p) const
{
    p = mPos;
}
const GLfloat fogColor[] = {0.5f, 0.8f, 1.0f};
WorldRenderer::WorldRenderer(Client *pCl):
    pClient(pCl)
{
    fogShaderProgram = CreateShaderProgram(GL_VERTEX_SHADER, fogVSH,
                                           GL_FRAGMENT_SHADER, fogFSH);
}
WorldRenderer::~WorldRenderer(void)
{
    glDeleteProgram(fogShaderProgram);
}
bool InCircle(const vec2 &p, const vec2 &center, const float radius)
{
    return pow(p.x - center.x, 2) + pow(p.y - center.y, 2) <= pow(radius, 2);
}
void WorldRenderer::RenderTerrain(const ChunkRenderCache &chunks,
                                  const Camera *pCamera,
                                  const float renderDistance)
{
    GLint loc;
    matrix4 matProj,
            matView;

    glUseProgram(fogShaderProgram);

    loc = glGetUniformLocation(fogShaderProgram, "fogColor");
    glUniform3fv(loc, 1, fogColor);

    loc = glGetUniformLocation(fogShaderProgram, "maxRenderDist");
    glUniform1f(loc, renderDistance);

    glGetFloatv(GL_PROJECTION_MATRIX, &matProj);
    glGetFloatv(GL_MODELVIEW_MATRIX, &matView);

    loc = glGetUniformLocation(fogShaderProgram, "projMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &matProj);

    loc = glGetUniformLocation(fogShaderProgram, "modelViewMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, &matView);

    vec3 cameraPos;
    pCamera->GetPosition(cameraPos);

    loc = glGetUniformLocation(fogShaderProgram, "cameraPos");
    glUniform3fv(loc, 1, cameraPos.v);


    ChunkCoord minCX = floor((cameraPos.x - renderDistance) / PER_CHUNK_SIZE),
               minCZ = floor((cameraPos.z - renderDistance) / PER_CHUNK_SIZE),
               maxCX = ceil((cameraPos.x + renderDistance) / PER_CHUNK_SIZE),
               maxCZ = ceil((cameraPos.z + renderDistance) / PER_CHUNK_SIZE),
               CX, CZ;

    vec2 chunkCenter;
    vec3 chunkOrigin,
         p1, p2;

    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    for (CX = minCX; CX <= maxCX; CX++)
    {
        for (CZ = minCZ; CZ <= maxCZ; CZ++)
        {
            ChunkID chunkID = MakeChunkID(CX, CZ);
            if (chunks.find(chunkID) == chunks.end())
                continue;

            const ChunkRenderObject *pChunk = chunks.at(chunkID);

            chunkOrigin.x = CX * PER_CHUNK_SIZE;
            chunkOrigin.z = CZ * PER_CHUNK_SIZE;
            chunkOrigin.y = 0.0f;

            chunkCenter.x = chunkOrigin.x + GLfloat(PER_CHUNK_SIZE) / 2;
            chunkCenter.z = chunkOrigin.z + GLfloat(PER_CHUNK_SIZE) / 2;

            if (InCircle(chunkCenter, {cameraPos.x, cameraPos.z}, renderDistance))
            {
                pChunk->Render();
            }
        }
    }

    glUseProgram(NULL);
}
#define VIEW_ANGLE 45.0f
#define NEAR_VIEW 0.1f
void WorldRenderer::RenderAll(const ChunkRenderCache &chunks, const Camera *pCamera)
{
    ClientSettings settings;
    pClient->GetSettings(settings);

    glMatrixMode(GL_PROJECTION);
    matrix4 mPerspec = MatPerspec(VIEW_ANGLE,
                                  (GLfloat)settings.display.resolution.width / (GLfloat)settings.display.resolution.height,
                                  NEAR_VIEW, GLfloat(settings.render.renderDistance));
    glLoadMatrixf(&mPerspec);
    glViewport(0, 0, settings.display.resolution.width,
                     settings.display.resolution.height);

    glMatrixMode(GL_MODELVIEW);
    matrix4 mView;
    pCamera->GetViewMatrix(mView);
    glLoadMatrixf(&mView);

    glClearColor(fogColor[0], fogColor[1], fogColor[2], 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderTerrain(chunks, pCamera, GLfloat(settings.render.renderDistance));
}
