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

enum KernelType
{
    OSD_KERNEL_CPU  = 0,  // CPU
    OSD_KERNEL_CL   = 1,  // OpenCL
    OSD_KERNEL_TBB  = 2,  // TBB
    OSD_KERNEL_GLSL = 3,  // GLSL
};

class CSubdivide
{
public:
    CSubdivide()
    {
        m_level    = 3;
        m_scheme   = Sdc::SCHEME_CATMARK;
        m_kernel   = OSD_KERNEL_CPU;
        m_adaptive = false;
        m_boundary = Sdc::Options::VTX_BOUNDARY_NONE;
        m_fvar     = Sdc::Options::FVAR_LINEAR_ALL;
        m_crease   = Sdc::Options::CREASE_UNIFORM;
        m_triangle = Sdc::Options::TRI_SUB_CATMARK;
        m_glmesh   = nullptr;
        m_refiner  = nullptr;
    }
    ~CSubdivide()
    {
        Clear();
    }

    bool Build(CLxUser_Mesh& mesh);
    bool Refine();
    bool Apply(CLxUser_Mesh& edit_mesh);
    void Clear();

    // OpenSubdiv options
    unsigned m_level;
    unsigned m_kernel;
    bool     m_adaptive;

    Sdc::SchemeType m_scheme;
    Sdc::Options::VtxBoundaryInterpolation m_boundary;
    Sdc::Options::FVarLinearInterpolation  m_fvar;
    Sdc::Options::CreasingMethod           m_crease;
    Sdc::Options::TriangleSubdivision      m_triangle;

    std::vector<float> m_position;  // subdivided positions

private:
    bool SetupCages();
    bool CreateSubdivPoints(std::vector<LXtPointID>& points);
    bool CreateSubdivPolygons(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons);
    int  GetCagePolygon(int face, CLxUser_Polygon& upoly);
    void RemoveSourcePolygons();

    Far::TopologyRefiner* CreateTopologyRefiner();
    Osd::GLMeshInterface* CreateMeshInterface (Far::TopologyRefiner* refiner);

    CLxUser_Mesh        m_mesh;
    CLxUser_Mesh        m_edit_mesh;
    CLxUser_LogService  s_log;
    CLxUser_MeshService s_mesh;

    Osd::GLMeshInterface* m_glmesh;
    Far::TopologyRefiner* m_refiner;

    std::vector<LXtPolygonID> m_polygons;
    std::vector<LXtPointID>   m_points;
    std::vector<FaceVarying>  m_fvarArray;
    std::map<LXtPolygonID,int> m_polyFaceMap;
    MeshBinning               m_mbin;
};