//
// OpenSubdiv mesh modifier named "mesh.opensubdiv.item"
//

#pragma once

#include <lxsdk/lxu_attributes.hpp>
#include <lxsdk/lxu_math.hpp>
#include <lxsdk/lxu_matrix.hpp>
#include <lxsdk/lxu_modifier.hpp>
#include <lxsdk/lxu_package.hpp>
#include <lxsdk/lxu_select.hpp>
#include <lxsdk/lxu_vector.hpp>

#include <lxsdk/lx_mesh.hpp>
#include <lxsdk/lx_package.hpp>
#include <lxsdk/lx_particle.hpp>
#include <lxsdk/lx_plugin.hpp>
#include <lxsdk/lx_pmodel.hpp>
#include <lxsdk/lx_seltypes.hpp>
#include <lxsdk/lx_server.hpp>
#include <lxsdk/lx_vector.hpp>
#include <lxsdk/lx_vertex.hpp>
#include <lxsdk/lx_visitor.hpp>
#include <lxsdk/lx_wrap.hpp>

#include <lxsdk/lxw_mesh.hpp>
#include <lxsdk/lxw_pmodel.hpp>
#include <lxsdk/lxvalue.h>

#include <opensubdiv/osd/cpuEvaluator.h>

#include "subdivide.hpp"

using namespace lx_err;

#define SRVNAME "meshop.osd"

#define ARGs_LEVEL    "level"
#define ARGs_SCHEME   "scheme"
#define ARGs_BOUNDARY "boundary"
#define ARGs_FVAR     "fvar"
#define ARGs_CREASE   "crease"
#define ARGs_TRIANGLE "triangle"

#define ARGi_LEVEL     0
#define ARGi_SCHEME    1
#define ARGi_BOUNDARY  2
#define ARGi_FVAR      3
#define ARGi_CREASE    4
#define ARGi_TRIANGLE  5

#ifndef LXx_OVERRIDE
#define LXx_OVERRIDE override
#endif

class CMeshOp : public CLxImpl_MeshOperation, public CLxDynamicAttributes
{
public:
    CLxUser_MeshService  msh_S;
    LXtMarkMode          select_mode;

    CMeshOp()
    {
        static LXtTextValueHint subdivide_scheme[] = {
            { 0, "bilinear" }, 
            { 1, "catmull-clark" }, 
            { 2, "loop" },  0
        };
        static LXtTextValueHint subdivide_boundary[] = {
            { 0, "none" }, 
            { 1, "edgeOnly" }, 
            { 2, "edgeAndCorner" },  0
        };
        static LXtTextValueHint subdivide_fvar[] = {
            { 0, "none" }, 
            { 1, "cornersOnly" }, 
            { 2, "cornersPlus1" }, 
            { 3, "cornersPlus2" }, 
            { 4, "boundaries" }, 
            { 5, "all" },  0
        };
        static LXtTextValueHint subdivide_crease[] = {
            { 0, "uniform" }, 
            { 1, "chaikin" },  0
        };

        dyna_Add(ARGs_LEVEL, LXsTYPE_INTEGER);
        attr_SetInt(ARGi_LEVEL, 2);
    
        dyna_Add(ARGs_SCHEME, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_SCHEME, subdivide_scheme);
        attr_SetInt(ARGi_SCHEME, 1);
    
        dyna_Add(ARGs_BOUNDARY, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_BOUNDARY, subdivide_boundary);
        attr_SetInt(ARGi_BOUNDARY, 0);
    
        dyna_Add(ARGs_FVAR, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_FVAR, subdivide_fvar);
        attr_SetInt(ARGi_FVAR, 0);
    
        dyna_Add(ARGs_CREASE, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_CREASE, subdivide_crease);
        attr_SetInt(ARGi_CREASE, 1);
    
        dyna_Add(ARGs_TRIANGLE, LXsTYPE_BOOLEAN);
        attr_SetInt(ARGi_TRIANGLE, 0);

        select_mode = msh_S.SetMode(LXsMARK_SELECT);
    }

    static void initialize()
    {
        CLxGenericPolymorph* srv = new CLxPolymorph<CMeshOp>;

        srv->AddInterface(new CLxIfc_MeshOperation<CMeshOp>);
        srv->AddInterface(new CLxIfc_Attributes<CMeshOp>);
        srv->AddInterface(new CLxIfc_StaticDesc<CMeshOp>);

        lx::AddServer(SRVNAME, srv);
    }

    LXxO_MeshOperation_Evaluate;
    LXxO_MeshOperation_ReEvaluate;
    LXxO_MeshOperation_Compare;
    LXxO_MeshOperation_Convert;
	LXxO_MeshOperation_SetTransform;

    static LXtTagInfoDesc descInfo[];

    //
    // Subdivision context
    //
    CSubdivide m_csub;                                     
};
