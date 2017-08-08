#include <map>

#include "boost/polygon/voronoi.hpp"
namespace pg =  boost::polygon;

#include "voronoi.h"
#include "log.h"


#define VORONOI_PRECISION 100000

typedef signed long long VoronoiCoord;

struct VoronoiPoint
{
    VoronoiCoord x, y;
};

namespace boost { namespace polygon {

template <>
struct geometry_concept<VoronoiPoint>
{
    typedef point_concept type;
};

template <>
struct point_traits<VoronoiPoint>
{
    typedef VoronoiCoord coordinate_type;

    static inline coordinate_type get(const VoronoiPoint& point, orientation_2d orient) {
        return (orient == HORIZONTAL) ? point.x : point.y;
    }
};

}}

void GetVoronoi(const std::vector<vec2> points, VoronoiDiagram &vdOut)
{
    std::vector<VoronoiPoint> integerPoints(points.size(), {0, 0});
    for (size_t i = 0; i < points.size(); i++)
    {
        integerPoints[i].x = VoronoiCoord(points[i].x * VORONOI_PRECISION);
        integerPoints[i].y = VoronoiCoord(points[i].y * VORONOI_PRECISION);
    }

    pg::voronoi_diagram<double> vd;
    pg::construct_voronoi(integerPoints.begin(), integerPoints.end(), &vd);

    std::map <const pg::voronoi_diagram<double>::cell_type *, VoronoiCell *> mapCells;
    for (const pg::voronoi_diagram<double>::cell_type &cell : vd.cells())
    {
        vdOut.mCells.push_back(VoronoiCell());
        mapCells[&cell] = &(vdOut.mCells.back());

        vdOut.mCells.back().mPoint = points[cell.source_index()];
    }

    std::map <const pg::voronoi_diagram<double>::edge_type *, VoronoiEdge *> mapEdges;
    for (const pg::voronoi_diagram<double>::edge_type &edge : vd.edges())
    {
        if (mapEdges.find(edge.twin()) == mapEdges.end())
        {
            vdOut.mEdges.push_back(VoronoiEdge());
            mapEdges[&edge] = &(vdOut.mEdges.back());

            vdOut.mEdges.back().v0 = NULL;
            vdOut.mEdges.back().v1 = NULL;

            vdOut.mEdges.back().c0 = NULL;
            vdOut.mEdges.back().c1 = NULL;
        }
    }

    std::map <const pg::voronoi_diagram<double>::vertex_type *, VoronoiVertex *> mapVertices;
    for (const pg::voronoi_diagram<double>::vertex_type &vertex : vd.vertices())
    {
        vdOut.mVertices.push_back(VoronoiVertex());
        mapVertices[&vertex] = &(vdOut.mVertices.back());

        vdOut.mVertices.back().x = GLfloat(vertex.x()) / VORONOI_PRECISION;
        vdOut.mVertices.back().y = GLfloat(vertex.y()) / VORONOI_PRECISION;
    }

    // Connect:
    for (const pg::voronoi_diagram<double>::cell_type &cell : vd.cells())
    {
        VoronoiCell *rCell = mapCells.at(&cell);

        const pg::voronoi_diagram<double>::edge_type *edge = cell.incident_edge();

        do
        {
            VoronoiEdge *rEdge;
            if (mapEdges.find(edge) == mapEdges.end())
                rEdge = mapEdges.at(edge->twin());
            else
                rEdge = mapEdges.at(edge);

            rCell->pEdges.push_back(rEdge);

            if (rEdge->c0 == NULL)
                rEdge->c0 = rCell;
            else
                rEdge->c1 = rCell;

            if (edge->vertex0() != NULL)
            {
                VoronoiVertex *rVertex = mapVertices.at(edge->vertex0());

                rEdge->v0 = rVertex;
                rVertex->pEdges.push_back(rEdge);
                rVertex->pCells.push_back(rCell);
            }

            if (edge->vertex1() != NULL)
            {
                VoronoiVertex *rVertex = mapVertices.at(edge->vertex1());

                rEdge->v1 = rVertex;
                rVertex->pEdges.push_back(rEdge);
                rVertex->pCells.push_back(rCell);
            }

            edge = edge->next();
        }
        while (edge != cell.incident_edge());
    }
}
