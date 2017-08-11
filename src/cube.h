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

#ifndef CUBE_H
#define CUBE_H

#include "exception.h"
#include "log.h"


template <typename T>
void Swap(T &t1, T &t2)
{
    T tmp = t1;
    t1 = t2;
    t2 = tmp;
}


// x,y,z as bits, either 0 or 1
typedef uint8_t UnitCubeVertexIndex;

inline UnitCubeVertexIndex UnitCubeVertex(uint8_t x, uint8_t y, uint8_t z)
{
    uint8_t i = 0;
    if (x > 0)
        i |= 0x01;
    if (y > 0)
        i |= 0x02;
    if (z > 0)
        i |= 0x04;

    return i;
}

uint8_t GetY(UnitCubeVertexIndex v)
{
    return ((v & 0x02) >> 1);
}

#define N_CUBE_VERTICES 8
#define UNITCUBE_VERTEX_I000 0x00
#define UNITCUBE_VERTEX_I100 0x01
#define UNITCUBE_VERTEX_I010 0x02
#define UNITCUBE_VERTEX_I110 0x03
#define UNITCUBE_VERTEX_I001 0x04
#define UNITCUBE_VERTEX_I101 0x05
#define UNITCUBE_VERTEX_I011 0x06
#define UNITCUBE_VERTEX_I111 0x07


typedef uint32_t UnitCubeEdgeIndex;

union UnitCubeEdge
{
    UnitCubeVertexIndex v[2];
    UnitCubeEdgeIndex id;
};

#define N_CUBE_EDGES 12
const UnitCubeEdge unitCubeEdges[N_CUBE_EDGES] = {{UNITCUBE_VERTEX_I000, UNITCUBE_VERTEX_I100},
                                                  {UNITCUBE_VERTEX_I000, UNITCUBE_VERTEX_I010},
                                                  {UNITCUBE_VERTEX_I000, UNITCUBE_VERTEX_I001},
                                                  {UNITCUBE_VERTEX_I100, UNITCUBE_VERTEX_I110},
                                                  {UNITCUBE_VERTEX_I100, UNITCUBE_VERTEX_I101},
                                                  {UNITCUBE_VERTEX_I010, UNITCUBE_VERTEX_I110},
                                                  {UNITCUBE_VERTEX_I010, UNITCUBE_VERTEX_I011},
                                                  {UNITCUBE_VERTEX_I001, UNITCUBE_VERTEX_I011},
                                                  {UNITCUBE_VERTEX_I001, UNITCUBE_VERTEX_I101},
                                                  {UNITCUBE_VERTEX_I110, UNITCUBE_VERTEX_I111},
                                                  {UNITCUBE_VERTEX_I101, UNITCUBE_VERTEX_I111},
                                                  {UNITCUBE_VERTEX_I011, UNITCUBE_VERTEX_I111}};


#define N_CUBE_AXES 3
#define N_CUBE_EDGES_PER_AXIS 4
/**
 * Returns always cw or always ccw, depending on the coordinate system.
 * (ccw in right-handed system)
 */
const UnitCubeEdge axisEdges[N_CUBE_AXES][N_CUBE_EDGES_PER_AXIS] = {{{UNITCUBE_VERTEX_I000, UNITCUBE_VERTEX_I100},
                                                                     {UNITCUBE_VERTEX_I001, UNITCUBE_VERTEX_I101},
                                                                     {UNITCUBE_VERTEX_I011, UNITCUBE_VERTEX_I111},
                                                                     {UNITCUBE_VERTEX_I010, UNITCUBE_VERTEX_I110}},
                                                                    {{UNITCUBE_VERTEX_I000, UNITCUBE_VERTEX_I010},
                                                                     {UNITCUBE_VERTEX_I100, UNITCUBE_VERTEX_I110},
                                                                     {UNITCUBE_VERTEX_I101, UNITCUBE_VERTEX_I111},
                                                                     {UNITCUBE_VERTEX_I001, UNITCUBE_VERTEX_I011}},
                                                                    {{UNITCUBE_VERTEX_I000, UNITCUBE_VERTEX_I001},
                                                                     {UNITCUBE_VERTEX_I010, UNITCUBE_VERTEX_I011},
                                                                     {UNITCUBE_VERTEX_I110, UNITCUBE_VERTEX_I111},
                                                                     {UNITCUBE_VERTEX_I100, UNITCUBE_VERTEX_I101}}};

typedef uint8_t UnitCubeAxis;
#define UNITCUBE_AXIS_X 0x01
#define UNITCUBE_AXIS_Y 0x02
#define UNITCUBE_AXIS_Z 0x04

#define N_CUBE_FACES 6

struct UnitCubeFace
{
    UnitCubeAxis axis;
    uint8_t value;  // 0 or 1 on axis
};

const UnitCubeFace unitCubeFaces[N_CUBE_FACES] = {{UNITCUBE_AXIS_X, 0}, {UNITCUBE_AXIS_X, 1},
                                                  {UNITCUBE_AXIS_Y, 0}, {UNITCUBE_AXIS_Y, 1},
                                                  {UNITCUBE_AXIS_Z, 0}, {UNITCUBE_AXIS_Z, 1}};


bool EdgesConnected(const UnitCubeEdge &e1, const UnitCubeEdge &e2)
{
    return e1.v[0] == e2.v[0] || e1.v[0] == e2.v[1] ||
           e1.v[1] == e2.v[0] || e1.v[1] == e2.v[1];
}

bool VertexInFace(const UnitCubeVertexIndex &vertex, const UnitCubeFace &face)
{
    if (face.value != 0)
        return vertex & face.axis;
    else
        return !(vertex & face.axis);
}

bool EdgeInFace(const UnitCubeEdge &edge, const UnitCubeFace &face)
{
    return VertexInFace(edge.v[0], face) && VertexInFace(edge.v[1], face);
}

bool EdgesShareFace(const UnitCubeEdge &e1, const UnitCubeEdge &e2)
{
    for (int i = 0; i < N_CUBE_FACES; i++)
        if (EdgeInFace(e1, unitCubeFaces[i]) && EdgeInFace(e2, unitCubeFaces[i]))
            return true;

    return false;
}

int LookupEdge(const UnitCubeEdge &e)
{
    for (int i = 0; i < N_CUBE_EDGES; i++)
    {
        if (unitCubeEdges[i].v[0] == e.v[0] && unitCubeEdges[i].v[1] == e.v[1] ||
            unitCubeEdges[i].v[0] == e.v[1] && unitCubeEdges[i].v[0] == e.v[1])
            return i;
    }

    throw FormatableException("not a cube edge: 0x%x-0x%x", e.v[0], e.v[1]);
}

/**
 * Returns always cw or always ccw, depending on the coordinate system.
 * (ccw in right-handed system)
 */
inline void Get3EdgesFrom(const UnitCubeVertexIndex v, UnitCubeEdge *edges)
{
    UnitCubeVertexIndex dx = (~(v & 0x01)) & 0x01,
                        dy = (~(v & 0x02)) & 0x02,
                        dz = (~(v & 0x04)) & 0x04;

    if (dx && dy && dz)
    {
        edges[0] = {v, v | 0x01};
        edges[1] = {v, v | 0x02};
        edges[2] = {v, v | 0x04};
    }
    else if (!dx && dy && dz)
    {
        edges[0] = {v, v & ~0x01};
        edges[1] = {v, v | 0x04};
        edges[2] = {v, v | 0x02};
    }
    else if (dx && !dy && dz)
    {
        edges[0] = {v, v | 0x04};
        edges[1] = {v, v & ~0x02};
        edges[2] = {v, v | 0x01};
    }
    else if (dx && dy && !dz)
    {
        edges[0] = {v, v | 0x02};
        edges[1] = {v, v | 0x01};
        edges[2] = {v, v & ~0x04};
    }
    else if (dx && !dy && !dz)
    {
        edges[0] = {v, v | 0x01};
        edges[1] = {v, v & ~0x02};
        edges[2] = {v, v & ~0x04};
    }
    else if (!dx && dy && !dz)
    {
        edges[0] = {v, v & ~0x01};
        edges[1] = {v, v | 0x02};
        edges[2] = {v, v & ~0x04};
    }
    else if (!dx && !dy && dz)
    {
        edges[0] = {v, v & ~0x01};
        edges[1] = {v, v & ~0x02};
        edges[2] = {v, v | 0x04};
    }
    else if (!dx && !dy && !dz)
    {
        edges[0] = {v, v & ~0x01};
        edges[1] = {v, v & ~0x02};
        edges[2] = {v, v & ~0x04};
    }
}

/**
  * Returns always cw or always ccw, depending on the coordinate system.
  * (ccw in right-handed system)
  */
inline void Get4EdgesFrom(const UnitCubeEdge &edge, UnitCubeEdge *edges)
{
    UnitCubeVertexIndex edgeAxis = edge.v[0] ^ edge.v[1],
                        axis1, axis2, start;

    if (edge.v[1] & edgeAxis)
        start = edge.v[0];
    else
        start = edge.v[1];

    if (edgeAxis == 0x01)
    {
        axis1 = edge.v[0] & 0x02;
        axis2 = edge.v[0] & 0x04;
    }
    else if (edgeAxis == 0x02)
    {
        axis1 = edge.v[0] & 0x01;
        axis2 = edge.v[0] & 0x04;
    }
    else if (edgeAxis == 0x04)
    {
        axis1 = edge.v[0] & 0x01;
        axis2 = edge.v[0] & 0x02;
    }

    if ((axis1 && axis2) || (!axis1 && !axis2))
        Swap(axis1, axis2);

    edges[0] = {start, start ^ axis1};
    edges[1] = {start, start ^ axis2};
    edges[2] = {start | edgeAxis, start ^ axis2};
    edges[3] = {start | edgeAxis, start ^ axis1};
}

#endif  // CUBE_H
