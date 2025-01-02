//
// Subdivide class to wrap OpenSubdiv library.
//
#pragma once

#include <lxsdk/lxlog.h>
#include <lxsdk/lxu_geometry.hpp>
#include <lxsdk/lxu_scene.hpp>

#include <lxsdk/lx_poly.hpp>
#include <lxsdk/lx_surface.hpp>
#include <lxsdk/lx_value.hpp>
#include <lxsdk/lx_vertex.hpp>
#include <lxsdk/lx_visitor.hpp>
#include <lxsdk/lx_vmodel.hpp>

#include <lxsdk/lx_mesh.hpp>
#include <lxsdk/lxidef.h>
#include <lxsdk/lxu_math.hpp>
#include <lxsdk/lxu_vector.hpp>

#include <map>
#include <vector>

//
// Edge Visitor to get the edge creases from the source edge map.
//
class EdgeVisitor : public CLxImpl_AbstractVisitor
{
public:
    std::vector<float>&        m_edgeCreases;
    std::vector<int>&          m_edgeCreaseIndices;
    std::map<LXtPointID, int>& m_pointIndexMap;

    LXtMeshMapID   m_mapID;
    CLxUser_Point& m_point;
    CLxUser_Edge&  m_edge;

    EdgeVisitor(CLxLoc_MeshMap&            umap,
                CLxUser_Point&             point,
                CLxUser_Edge&              edge,
                std::vector<float>&        edgeCreases,
                std::vector<int>&          edgeCreaseIndices,
                std::map<LXtPointID, int>& pointIndexMap)
        : m_edgeCreases(edgeCreases), m_edgeCreaseIndices(edgeCreaseIndices), m_pointIndexMap(pointIndexMap), m_point(point), m_edge(edge)
    {
        m_mapID = umap.ID();
    }

    LxResult Evaluate() override
    {
        LXtPointID   p0, p1;
        unsigned int i0, i1;
        float        value;
        LxResult     res;

        m_edge.Endpoints(&p0, &p1);
        res = m_edge.MapValue(m_mapID, &value);
        if (res == LXe_OK)
        {
            auto it = m_pointIndexMap.find(p0);
            if (it == m_pointIndexMap.end())
                return LXe_OK;
            i0 = (unsigned int) it->second;

            it = m_pointIndexMap.find(p1);
            if (it == m_pointIndexMap.end())
                return LXe_OK;
            i1 = (unsigned int) it->second;

            m_edgeCreases.push_back(value * 10.f);
            m_edgeCreaseIndices.push_back(i0);
            m_edgeCreaseIndices.push_back(i1);
        }

        return LXe_OK;
    }
};

class EdgeMarkVisitor : public CLxImpl_AbstractVisitor
{
public:
    LXtMarkMode  m_mask;
    CLxUser_Edge m_edge;

    LxResult Evaluate() override
    {
        m_edge.SetMarks(m_mask);
        return LXe_OK;
    }
};

//
// Vertex Visitor to get the vertex creases from the source vertex map.
//
class VertexVisitor : public CLxImpl_AbstractVisitor
{
public:
    std::vector<float>&        m_vtxCreases;
    std::vector<int>&          m_vtxCreaseIndices;
    std::map<LXtPointID, int>& m_pointIndexMap;

    LXtMeshMapID   m_mapID;
    CLxUser_Point& m_point;

    VertexVisitor(CLxLoc_MeshMap&            umap,
                  CLxUser_Point&             point,
                  std::vector<float>&        vtxCreases,
                  std::vector<int>&          vtxCreaseIndices,
                  std::map<LXtPointID, int>& pointIndexMap)
        : m_vtxCreases(vtxCreases), m_vtxCreaseIndices(vtxCreaseIndices), m_pointIndexMap(pointIndexMap), m_point(point)
    {
        m_mapID = umap.ID();
    }

    LxResult Evaluate() override
    {
        unsigned int index;
        float        value;
        LxResult     res;

        res = m_point.MapValue(m_mapID, &value);
        if (res == LXe_OK)
        {
            auto it = m_pointIndexMap.find(m_point.ID());
            if (it == m_pointIndexMap.end())
                return LXe_OK;
            index = (unsigned int) it->second;
            m_vtxCreases.push_back(value * 10.f);
            m_vtxCreaseIndices.push_back(index);
        }

        return LXe_OK;
    }
};

//
// Get UV map values of the cage polygons from the source vertex map.
//
typedef struct st_FVarDisco
{
    unsigned int index;
    float        values[4];
} FVarDisco;

typedef struct st_FVarVertex
{
    std::vector<FVarDisco> discos;
} FVarVertex;

typedef struct
{
    LXtMeshMapID       m_mapID;
    std::string        name;
    std::vector<int>   indices;
    std::vector<float> values;
} FaceVarying;

class MeshMapVisitor : public CLxImpl_AbstractVisitor
{
public:
    CLxUser_Mesh&              m_mesh;
    CLxUser_MeshMap            m_maps;
    unsigned int               m_count;
    std::vector<FaceVarying>&  m_fvarArray;
    std::vector<LXtPolygonID>& m_polygons;

    MeshMapVisitor(CLxUser_Mesh& mesh, std::vector<FaceVarying>& fvarArray, std::vector<LXtPolygonID>& polygons)
        : m_mesh(mesh), m_fvarArray(fvarArray), m_polygons(polygons)
    {
        m_count = 0;
        m_mesh.GetMaps(m_maps);
        m_maps.FilterByType(LXi_VMAP_TEXTUREUV);
    }

    bool LookupDisco(std::vector<FVarDisco>& discos, unsigned int dim, const float* value, unsigned int* index)
    {
        unsigned int i, j;

        for (i = 0; i < discos.size(); i++)
        {
            for (j = 0; j < dim; j++)
            {
                if (lx::Compare(discos[i].values[j], value[j]) != 0)
                    break;
            }
            if (j == dim)
            {
                *index = discos[i].index;
                return true;
            }
        }
        return false;
    }

    bool CreateFaceVaryTable(CLxUser_Mesh&              mesh,
                             CLxUser_MeshMap&           umap,
                             std::vector<LXtPolygonID>& polyIDs,
                             std::vector<int>&          indices,
                             std::vector<float>&        values)
    {
        unsigned int            count, index, dim, ii, total = 0;
        std::vector<FVarVertex> fvarTable;
        FVarDisco               disco;
        CLxUser_Point           upoint;
        CLxUser_Polygon         upoly;
        LXtPointID              point;
        float                   value[4];
        LxResult                res;

        fvarTable.resize(mesh.NPoints());

        umap.Dimension(&dim);
        if (dim > 4)
            return false;

        mesh.GetPoints(upoint);
        mesh.GetPolygons(upoly);

        for (unsigned int i = 0; i < polyIDs.size(); i++)
        {
            upoly.Select(polyIDs[i]);
            upoly.VertexCount(&count);
            for (unsigned int j = 0; j < count; j++)
            {
                upoly.VertexByIndex(j, &point);
                res = upoly.MapEvaluate(umap.ID(), point, value);
                if (res != LXe_OK)
                {
                    indices.push_back(0);
                    continue;
                }
                upoint.Select(point);
                upoint.Index(&ii);
                if (LookupDisco(fvarTable[ii].discos, dim, value, &index))
                    indices.push_back(index);
                else
                {
                    disco.index = total++;
                    for (unsigned int k = 0; k < dim; k++)
                    {
                        disco.values[k] = value[k];
                        values.push_back(value[k]);
                    }
                    indices.push_back(disco.index);
                    fvarTable[ii].discos.push_back(disco);
                }
            }
        }
        return values.size() > 0;
    }

    LxResult Evaluate() override
    {
        FaceVarying faceVary;

        if (CreateFaceVaryTable(m_mesh, m_maps, m_polygons, faceVary.indices, faceVary.values))
        {
            faceVary.m_mapID = m_maps.ID();
            const char* name;
            m_maps.Name(&name);
            faceVary.name = name;
            m_fvarArray.push_back(faceVary);
        }
        return LXe_OK;
    }
};

//
// Get the polygons and number of vertices of the polygons.
//
typedef struct
{
    CLxUser_SurfaceBin bin;             // surface bin
    int                npols;           // number of polygon
    int                polyVertCounts;  // number of vertex of polygons
    int                faceVertCounts;  // number of subdivided face
    int                ntris;           // number of triangle for Loop scheme
    int                triVertCounts;   // number of vertex of triangles for Loop scheme
} MeshBinning;

class PolygonMeshBinVisitor : public CLxImpl_AbstractVisitor
{
public:
    std::vector<LXtPolygonID>& m_polygons;

    CLxUser_Polygon& m_upoly;
    MeshBinning&     m_bin;

    PolygonMeshBinVisitor(CLxUser_Polygon& upoly, MeshBinning& bin, std::vector<LXtPolygonID>& polygons)
        : m_polygons(polygons), m_upoly(upoly), m_bin(bin)
    {
        m_bin.npols          = 0;
        m_bin.polyVertCounts = 0;
        m_bin.faceVertCounts = 0;
        m_bin.ntris          = 0;
        m_bin.triVertCounts  = 0;
    }

    bool TestPolygon(CLxUser_Polygon& poly)
    {
        unsigned int count;
        LXtID4       type;

        poly.VertexCount(&count);
        if (count < 3)
            return false;
        poly.Type(&type);
        if ((type != LXiPTYP_PSUB) && (type != LXiPTYP_SUBD) && (type != LXiPTYP_FACE))
            return false;

        return true;
    }

    LxResult Evaluate() override
    {
        if (TestPolygon(m_upoly))
        {
            m_bin.npols++;
            m_polygons.push_back(m_upoly.ID());
            unsigned int count;
            m_upoly.VertexCount(&count);
            m_bin.polyVertCounts += count;
            m_bin.ntris += count - 2;
            m_bin.triVertCounts += (count - 2) * 3;
        }
        return LXe_OK;
    }
};

//
// Visitor which clears marks on vertices, or set marks on the vertices of
// tagged polygons.
//
class MarkingVisitor : public CLxImpl_AbstractVisitor
{
public:
    CLxUser_Point   pnt;
    CLxUser_Polygon pol;
    LXtMarkMode     mask;
    LXtID4          type;
    bool            clear;

    //
    // Return true if the polygon is a valid polygon for subdivision.
    //
    bool TestPolygon(CLxUser_Polygon& poly)
    {
        unsigned int count;
        LXtID4       type;

        poly.VertexCount(&count);
        if (count < 3)
            return false;
        poly.Type(&type);
        if ((type != LXiPTYP_PSUB) && (type != LXiPTYP_SUBD) && (type != LXiPTYP_FACE))
            return false;

        return true;
    }

    LxResult Evaluate() override
    {
        if (clear)
        {
            pnt.SetMarks(mask);
            return LXe_OK;
        }

        if (TestPolygon(pol))
        {
            unsigned n;

            pol.VertexCount(&n);
            for (auto i = 0u; i < n; i++)
            {
                pnt.SelectPolygonVertex(pol.ID(), i);
                pnt.SetMarks(mask);
            }
        }

        return LXe_OK;
    }
};

//
// Visitor which collects the vertex IDs of the vertices with the specified mark.
//
class VertexTableVisitor : public CLxImpl_AbstractVisitor
{
public:
    CLxUser_Point            pnt;
    LXtMarkMode              mask;
    std::vector<LXtPointID>& m_points;

    VertexTableVisitor(std::vector<LXtPointID>& points) : m_points(points)
    {
        m_points.clear();
    }

    LxResult Evaluate() override
    {
        if (pnt.TestMarks(mask) == LXe_TRUE)
        {
            m_points.push_back(pnt.ID());
        }

        return LXe_OK;
    }
};
