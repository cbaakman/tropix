#include <stdlib.h>
#include <math.h>
#include <map>


/*
void GetClosestNode(vec2 &node, const vec2 &p, const float gridSize)
{
    float xmin, xmax, ymin, ymax;

    xmin = gridSize * floor(p.x / gridSize);
    xmax = gridSize * ceil(p.x / gridSize);
    ymin = gridSize * floor(p.y / gridSize);
    ymax = gridSize * ceil(p.y / gridSize);

    if (fabs(p.x - xmin) < fabs(p.x - xmax))
        node.x = xmin;
    else
        node.x = xmax;

    if (fabs(p.y - ymin) < fabs(p.y - ymax))
        node.y = ymin;
    else
        node.y = ymax;
}
#define RANDFLOAT_PRECISION 10000
void GetPointsInSquare(WorldSeed seed, int n,
                       const float x1, const float y1,
                       const float x2, const float y2,
                       std::list<vec2> &points)
{
    WorldSeed r;
    float f;
    vec2 p;

    for (int i = 0; i < n; i++)
    {
        seed = WorldSeed(9185162 * (seed * x1 + y1)) + 2471575;
        r = seed % RAND_MAX;
        f = float(r % RANDFLOAT_PRECISION) / RANDFLOAT_PRECISION;
        // f is between 0.0f and 1.0f

        p.x = x1 + f * (x2 - x1);

        seed = WorldSeed(9798567 * (seed * y1 + x1)) + 4369387;
        r = seed % RAND_MAX;
        f = float(r % RANDFLOAT_PRECISION) / RANDFLOAT_PRECISION;
        // f is between 0.0f and 1.0f

        p.y = y1 + f * (y2 - y1);

        points.push_back(p);
    }
}
template <>
class SquarePointGenerator<2>
{
private:
    WorldSeed mSeed;
    float mSquareSize;
    int mPerSquare;
public:
    SquarePointGenerator(const WorldSeed seed,
                         const float squareSize,
                         const int perSquare):
        mSeed(seed),
        mSquareSize(squareSize),
        mPerSquare(perSquare)
    {
    }

    void GetPointsIn(const vec2 &center,
                     const float radius,
                     std::vector<vec2> &points) const
    {
        vec2 centerNode;
        float xmin, ymin, xmax, ymax,
              x1, y1;

        GetClosestNode(centerNode, center, mSquareSize);

        xmin = centerNode.x - radius - mSquareSize;
        xmax = centerNode.x + radius + mSquareSize;
        ymin = centerNode.y - radius - mSquareSize;
        ymax = centerNode.y + radius + mSquareSize;

        for (x1 = xmin; x1 < xmax; x1 += mSquareSize)
        {
            for (y1 = ymin; y1 < ymax; y1 += mSquareSize)
            {
                std::list<vec2> candidates;

                GetPointsInSquare(mSeed, mPerSquare,
                                  x1, y1,
                                  x1 + mSquareSize,
                                  y1 + mSquareSize,
                                  candidates);
                for (const vec2 &candidate : candidates)
                {
                    if (InCircle(candidate, center, radius))
                        points.push_back(candidate);
                }
            }
        }
    }
};
float NoiseFrom(const int x)
{
    int y = (x << 13) ^ x;
    return (1.0 - ((y * (y * y * 15731 + 789221) + 1376312589) & 0x7fffffff)
            / 1073741824.0);
}
TerrainLandGenerator::TerrainLandGenerator(const uint32_t seed,
                                           const TerrainPointGenerator *pg):
    mSeed(seed),
    pPointGenerator(pg)
{
}
void TerrainLandGenerator::GetTerrain(const vec2 &center, const float radius,
                                      Terrain &terrain) const
{
    float p;
    int hue;

    std::vector<vec2> points;
    VoronoiDiagram vd;

    pPointGenerator->GetPointsIn(center, radius, points);
    GetVoronoi(points, vd);

    std::map<const VoronoiVertex *, TerrainVertex *> mapVertices;
    for (const VoronoiVertex &vertex : vd.mVertices)
    {
        terrain.mVertices.push_back({vec3(vertex.x, 0.0f, vertex.z)});
        mapVertices[&vertex] = &(terrain.mVertices.back());
    }

    for (const VoronoiCell &cell : vd.mCells)
    {
        terrain.mVertices.push_back({vec3(cell.mPoint.x, 0.0f, cell.mPoint.z)});

        TerrainVertex *pVertex = &(terrain.mVertices.back());

        p = 0.5f + 0.5f * NoiseFrom(NoiseFrom(NoiseFrom(mSeed) * 43569185 + cell.mPoint.x) * cell.mPoint.z);

        hue = (p <= LAND_CHANCE) ? 60 : 240;

        for (const VoronoiEdge *pEdge : cell.pEdges)
        {
            if (pEdge->v0 != NULL && pEdge->v1 != NULL)
            {
                if (InCircle(*(pEdge->v0), center, radius) &&
                    InCircle(*(pEdge->v1), center, radius))
                {
                    TerrainFace face = {pVertex,
                                        mapVertices.at(pEdge->v0),
                                        mapVertices.at(pEdge->v1),
                                        hue};

                    terrain.mFaces.push_back(face);
                }
            }
        }
    }
}
TerrainRockGenerator::TerrainRockGenerator(const uint32_t seed,
                                           const TerrainPointGenerator *pg):
    mSeed(seed),
    pPointGenerator(pg)
{
}
void TerrainRockGenerator::GetTerrain(const vec2 &center, const float radius,
                                      Terrain &terrain) const
{
    int hue;

    std::vector<vec2> points;
    VoronoiDiagram vd;

    pPointGenerator->GetPointsIn(center, radius, points);
    GetVoronoi(points, vd);

    std::map<const VoronoiVertex *, TerrainVertex *> mapVertices;
    for (const VoronoiVertex &vertex : vd.mVertices)
    {
        terrain.mVertices.push_back({vec3(vertex.x, 0.0f, vertex.z)});
        mapVertices[&vertex] = &(terrain.mVertices.back());
    }

    for (const VoronoiCell &cell : vd.mCells)
    {
        terrain.mVertices.push_back({vec3(cell.mPoint.x, 0.0f, cell.mPoint.z)});

        TerrainVertex *pVertex = &(terrain.mVertices.back());

        hue = 180 + 180 * sin(NoiseFrom(NoiseFrom(NoiseFrom(mSeed) + cell.mPoint.x) * cell.mPoint.z));

        for (const VoronoiEdge *pEdge : cell.pEdges)
        {
            if (pEdge->v0 != NULL && pEdge->v1 != NULL)
            {
                if (InCircle(*(pEdge->v0), center, radius) &&
                    InCircle(*(pEdge->v1), center, radius))
                {
                    TerrainFace face = {pVertex,
                                        mapVertices.at(pEdge->v0),
                                        mapVertices.at(pEdge->v1),
                                        hue};

                    terrain.mFaces.push_back(face);
                }
            }
        }
    }
}
*/
