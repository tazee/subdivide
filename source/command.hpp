//
// The command version of subdivision using OpenSubdiv.
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

#define SRVNAME_COMMAND "poly.osd"


#define ARGs_COMMAND_LEVEL    "level"
#define ARGs_COMMAND_SCHEME   "scheme"
#define ARGs_COMMAND_ADAPTIVE "adaptive"
#define ARGs_COMMAND_BOUNDARY "boundary"
#define ARGs_COMMAND_FVAR     "fvar"
#define ARGs_COMMAND_CREASE   "crease"
#define ARGs_COMMAND_TRIANGLE "triangle"

#define ARGi_COMMAND_LEVEL     0
#define ARGi_COMMAND_SCHEME    1
#define ARGi_COMMAND_ADAPTIVE  2
#define ARGi_COMMAND_BOUNDARY  3
#define ARGi_COMMAND_FVAR      4
#define ARGi_COMMAND_CREASE    5
#define ARGi_COMMAND_TRIANGLE  6

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

        dyna_Add(ARGs_COMMAND_LEVEL, LXsTYPE_INTEGER);
        attr_SetInt(ARGi_COMMAND_LEVEL, 1);
    
        dyna_Add(ARGs_COMMAND_SCHEME, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_COMMAND_SCHEME, subdivide_scheme);
        attr_SetInt(ARGi_COMMAND_SCHEME, 1);
    
        dyna_Add(ARGs_COMMAND_ADAPTIVE, LXsTYPE_BOOLEAN);
        attr_SetInt(ARGi_COMMAND_ADAPTIVE, 0);
    
        dyna_Add(ARGs_COMMAND_BOUNDARY, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_COMMAND_BOUNDARY, subdivide_boundary);
        attr_SetInt(ARGi_COMMAND_BOUNDARY, 0);
    
        dyna_Add(ARGs_COMMAND_FVAR, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_COMMAND_FVAR, subdivide_fvar);
        attr_SetInt(ARGi_COMMAND_FVAR, 0);
    
        dyna_Add(ARGs_COMMAND_CREASE, LXsTYPE_INTEGER);
        dyna_SetHint(ARGi_COMMAND_CREASE, subdivide_crease);
        attr_SetInt(ARGi_COMMAND_CREASE, 1);
    
        dyna_Add(ARGs_COMMAND_TRIANGLE, LXsTYPE_BOOLEAN);
        attr_SetInt(ARGi_COMMAND_TRIANGLE, 0);

        check(msh_S.ModeCompose("select", NULL, &select_mode));
    }

    static void initialize()
    {
        CLxGenericPolymorph* srv;

        srv = new CLxPolymorph<CCommand>;
        srv->AddInterface(new CLxIfc_Command<CCommand>);
        srv->AddInterface(new CLxIfc_Attributes<CCommand>);
        srv->AddInterface(new CLxIfc_AttributesUI<CCommand>);
        lx::AddServer(SRVNAME_COMMAND, srv);
    }

    int  basic_CmdFlags() LXx_OVERRIDE;
    bool basic_Enable(CLxUser_Message& msg) LXx_OVERRIDE;

    void basic_Execute(unsigned int flags) LXx_OVERRIDE;

    LxResult cmd_DialogInit(void) LXx_OVERRIDE;
};
