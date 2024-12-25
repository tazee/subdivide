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

using namespace lx_err;

#define SRVNAME_MESHOP "meshop.osd"

#define ARGs_MESHOP_LEVEL    "level"
#define ARGs_MESHOP_SCHEME   "scheme"
#define ARGs_MESHOP_ADAPTIVE "adaptive"
#define ARGs_MESHOP_BOUNDARY "boundary"
#define ARGs_MESHOP_FVAR     "fvar"
#define ARGs_MESHOP_CREASE   "crease"
#define ARGs_MESHOP_TRIANGLE "triangle"

#define ARGi_MESHOP_LEVEL     0
#define ARGi_MESHOP_SCHEME    1
#define ARGi_MESHOP_ADAPTIVE  2
#define ARGi_MESHOP_BOUNDARY  3
#define ARGi_MESHOP_FVAR      4
#define ARGi_MESHOP_CREASE    5
#define ARGi_MESHOP_TRIANGLE  6

#ifndef LXx_OVERRIDE
#define LXx_OVERRIDE override
#endif

class CMeshOp : public CLxImpl_MeshOperation, public CLxDynamicAttributes, public CLxImpl_MeshElementGroup
{
public:
    CLxUser_MeshService  msh_S;
    unsigned             select_mode;

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

        dyna_Add(ARGs_MESHOP_LEVEL, LXsTYPE_INTEGER);
        attr_SetInt(ARGi_MESHOP_LEVEL, 3);
    
        dyna_Add(ARGs_MESHOP_SCHEME, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_MESHOP_SCHEME, subdivide_scheme);
        attr_SetInt(ARGi_MESHOP_SCHEME, 1);
    
        dyna_Add(ARGs_MESHOP_ADAPTIVE, LXsTYPE_BOOLEAN);
        attr_SetInt(ARGi_MESHOP_ADAPTIVE, 0);
    
        dyna_Add(ARGs_MESHOP_BOUNDARY, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_MESHOP_BOUNDARY, subdivide_boundary);
        attr_SetInt(ARGi_MESHOP_BOUNDARY, 0);
    
        dyna_Add(ARGs_MESHOP_FVAR, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_MESHOP_FVAR, subdivide_fvar);
        attr_SetInt(ARGi_MESHOP_FVAR, 0);
    
        dyna_Add(ARGs_MESHOP_CREASE, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_MESHOP_CREASE, subdivide_crease);
        attr_SetInt(ARGi_MESHOP_CREASE, 1);
    
        dyna_Add(ARGs_MESHOP_TRIANGLE, LXsTYPE_BOOLEAN);
        attr_SetInt(ARGi_MESHOP_TRIANGLE, 0);

        check(msh_S.ModeCompose("select", NULL, &select_mode));
    }

    static void initialize()
    {
        CLxGenericPolymorph* srv = new CLxPolymorph<CMeshOp>;

        srv->AddInterface(new CLxIfc_MeshOperation<CMeshOp>);
        srv->AddInterface(new CLxIfc_Attributes<CMeshOp>);
        srv->AddInterface(new CLxIfc_StaticDesc<CMeshOp>);

        lx::AddServer(SRVNAME_MESHOP, srv);
    }

    LXxO_MeshOperation_Evaluate;

    static LXtTagInfoDesc descInfo[];
                                        
};
