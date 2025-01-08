//
// The command version of polygon subdivision using OpenSubdiv.
//

#pragma once

#include <lxsdk/lxu_command.hpp>

#include <lxsdk/lx_layer.hpp>
#include <lxsdk/lx_log.hpp>
#include <lxsdk/lxu_attributes.hpp>
#include <lxsdk/lxu_math.hpp>
#include <lxsdk/lxu_vector.hpp>

#include "lxsdk/lxvalue.h"
#include "subdivide.hpp"

using namespace lx_err;

#define SRVNAME "poly.osd"


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

class CCommand : public CLxBasicCommand
{
public:
    CLxUser_LayerService lyr_S;
    CLxUser_MeshService  msh_S;
    unsigned             select_mode;

    CCommand()
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
        static LXtTextValueHint triangle_subdivision[] = {
            { 0, "catmark" }, 
            { 1, "smooth" },  0
        };

        dyna_Add(ARGs_LEVEL, LXsTYPE_INTEGER);
        attr_SetInt(ARGi_LEVEL, 1);
    
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
    
        dyna_Add(ARGs_TRIANGLE, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_TRIANGLE, triangle_subdivision);
        attr_SetInt(ARGi_TRIANGLE, 1);

        select_mode = msh_S.SetMode(LXsMARK_SELECT);
    }

    static void initialize()
    {
        CLxGenericPolymorph* srv;

        srv = new CLxPolymorph<CCommand>;
        srv->AddInterface(new CLxIfc_Command<CCommand>);
        srv->AddInterface(new CLxIfc_Attributes<CCommand>);
        srv->AddInterface(new CLxIfc_AttributesUI<CCommand>);
        lx::AddServer(SRVNAME, srv);
    }

    int  basic_CmdFlags() LXx_OVERRIDE;
    bool basic_Enable(CLxUser_Message& msg) LXx_OVERRIDE;

    void basic_Execute(unsigned int flags) LXx_OVERRIDE;

    LxResult cmd_DialogInit(void) LXx_OVERRIDE;
};
