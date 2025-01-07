//
// Subdivide class to wrap OpenSubdiv library.
//
#pragma once

#include <lxsdk/lx_log.hpp>
#include <lxsdk/lx_mesh.hpp>
#include <lxsdk/lx_value.hpp>
#include <lxsdk/lxu_math.hpp>

#include <map>
#include <vector>
#include <memory>

#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/osd/cpuEvaluator.h>
#include <opensubdiv/osd/cpuGLVertexBuffer.h>
#include <opensubdiv/osd/glMesh.h>
#include <opensubdiv/sdc/options.h>

#include "visitors.hpp"

using namespace OpenSubdiv;

typedef Sdc::Options            Options;
typedef Far::TopologyDescriptor Descriptor;

class CSubdivide
{
public:
    CSubdivide()
    {
        m_level    = 3;
        m_scheme   = Sdc::SCHEME_CATMARK;
        m_boundary = Sdc::Options::VTX_BOUNDARY_NONE;
        m_fvar     = Sdc::Options::FVAR_LINEAR_ALL;
        m_crease   = Sdc::Options::CREASE_UNIFORM;
        m_triangle = Sdc::Options::TRI_SUB_CATMARK;
        m_refiner  = nullptr;
        m_pick     = 0;
    }
    ~CSubdivide()
    {
        Clear();
    }

    bool Build(CLxUser_Mesh& mesh, LXtMarkMode pick);
    bool Refine();
    bool Apply(CLxUser_Mesh& edit_mesh);
    bool Update();
    void Clear();

    void SetLevel (int level);
    void SetScheme (int scheme);
    void SetBoundary (int bounary);
    void SetFVar (int fvar);
    void SetCrease (int crease);
    void SetTriangle(int triangle);

    // OpenSubdiv options
    unsigned m_level;

    Sdc::SchemeType m_scheme;
    Sdc::Options::VtxBoundaryInterpolation m_boundary;
    Sdc::Options::FVarLinearInterpolation  m_fvar;
    Sdc::Options::CreasingMethod           m_crease;
    Sdc::Options::TriangleSubdivision      m_triangle;

private:
    bool SetupCages();
    bool CreateSubdivPoints(std::vector<LXtPointID>& points);
    bool CreateSubdivPolygons(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons);
    bool CreateSubdivUVs(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons);
    int  GetCagePolygon(int face, CLxUser_Polygon& upoly);
    void RemoveSourcePolygons();
    void RemoveSourcePoints();

    Far::TopologyRefiner* CreateTopologyRefiner();

    CLxUser_Mesh        m_mesh;
    CLxUser_Mesh        m_edit_mesh;
    CLxUser_LogService  s_log;
    CLxUser_MeshService s_mesh;

    Far::TopologyRefiner* m_refiner;

    std::vector<LXtPolygonID> m_polygons;   // source cage polygons
    std::vector<LXtPointID>   m_points;     // source points of cages
    std::vector<FaceVarying>  m_fvarArray;
    std::map<LXtPolygonID,int> m_polyFaceMap;
    MeshBinning               m_mbin;
    LXtMarkMode               m_pick;

    std::vector<float> m_position;  // subdivided positions
    std::vector<std::vector<float>> m_uvs;  // subdivided uv value array
    std::vector<LXtPointID> m_new_points;  // subdivided new points
};