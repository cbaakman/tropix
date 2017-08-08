#ifndef VORONOI_H
#define VORONOI_H

#include <vector>
#include <list>

#include "vec.h"

struct VoronoiCell;
struct VoronoiEdge;

struct VoronoiVertex : public vec2
{
    std::list<VoronoiCell *> pCells;
    std::list<VoronoiEdge *> pEdges;
};

struct VoronoiEdge
{
    VoronoiVertex *v0, *v1;
    VoronoiCell *c0, *c1;
};

struct VoronoiCell
{
    vec2 mPoint;
    std::list<VoronoiEdge *> pEdges;
};

struct VoronoiDiagram
{
    std::list<VoronoiVertex> mVertices;

    std::list<VoronoiEdge> mEdges;

    std::list<VoronoiCell> mCells;
};

void GetVoronoi(const std::vector<vec2> points, VoronoiDiagram &);

#endif  // VORONOI_H
