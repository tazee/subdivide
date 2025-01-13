//
// Subdivide class to wrap OpenSubdiv library.
//
#pragma once

#include <lxsdk/lx_log.hpp>
#include <lxsdk/lx_mesh.hpp>
#include <lxsdk/lxu_math.hpp>

#include <map>
#include <vector>
#include <memory>
#define _USE_MATH_DEFINES
#include <math.h>

#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>
#include <opensubdiv/sdc/options.h>

#include "visitors.hpp"

using namespace OpenSubdiv;

typedef Sdc::Options            Options;
typedef Far::TopologyDescriptor Descriptor;


//
// Vertex container implementation.
//
struct Point3 {

    // Minimal required interface ----------------------
    Point3() { }

    void Clear( void * =0 ) {
        _point[0] = _point[1] = _point[2] = 0.0f;
    }

    void AddWithWeight(Point3 const & src, float weight) {
        _point[0] += weight * src._point[0];
        _point[1] += weight * src._point[1];
        _point[2] += weight * src._point[2];
    }

    // Public interface ------------------------------------
    void SetPoint(float x, float y, float z) {
        _point[0] = x;
        _point[1] = y;
        _point[2] = z;
    }

    const float * GetPoint() const {
        return _point;
    }

private:
    float _point[3];
};
struct FVarValue {
    // Minimal required interface ----------------------
    void Clear() {
        for (unsigned i = 0; i < _value.size(); i++)
            _value[i] = 0.0f;
    }

    void AddWithWeight(FVarValue const & src, float weight) {
        _value.resize(src._value.size());
        for (unsigned i = 0; i < src._value.size(); i++)
            _value[i] += weight * src._value[i];
    }

    void SetValue(const float* value, unsigned size) {
        _value.resize(size);
        for (unsigned i = 0; i < size; i++)
            _value[i] = value[i];
    }

    // Vertex map value
    std::vector<float> _value;
};

class CSubdivide
{
public:
    CSubdivide()
    {
        m_level    = 2;
        m_scheme   = Sdc::SCHEME_CATMARK;
        m_boundary = Sdc::Options::VTX_BOUNDARY_NONE;
        m_fvar     = Sdc::Options::FVAR_LINEAR_NONE;
        m_crease   = Sdc::Options::CREASE_CHAIKIN;
        m_triangle = Sdc::Options::TRI_SUB_SMOOTH;
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

    bool TestPolygon(LXtPolygonID polID);
    bool TestEdge(LXtEdgeID edgeID);
    bool TestPoint(LXtPointID pntID);

    // OpenSubdiv options
    int m_level;

    Sdc::SchemeType m_scheme;
    Sdc::Options::VtxBoundaryInterpolation m_boundary;
    Sdc::Options::FVarLinearInterpolation  m_fvar;
    Sdc::Options::CreasingMethod           m_crease;
    Sdc::Options::TriangleSubdivision      m_triangle;

private:
    bool SetupCages();
    bool CreateSubdivPoints(std::vector<LXtPointID>& points);
    bool CreateSubdivPolygons(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons);
    bool CreateSubdivFVars(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons);
    bool CreateSubdivMorphs(std::vector<LXtPointID>& points);
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

    std::vector<Point3>                 m_subdiv_positions; // subdivided vertex positions
    std::vector<std::vector<FVarValue>> m_subdiv_fvars;     // subdivided vmap value array
    std::vector<LXtPointID>             m_subdiv_points;    // subdivided new points
    std::vector<LXtPolygonID>           m_subdiv_polygons;  // subdivided new polygons
};