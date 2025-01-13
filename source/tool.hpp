//
// OpenSubdiv mesh modifier named "tool.osd.item"
//

#pragma once

#include <lxsdk/lxu_attributes.hpp>
#include <lxsdk/lxu_select.hpp>

#include <lxsdk/lx_plugin.hpp>
#include <lxsdk/lx_seltypes.hpp>
#include <lxsdk/lx_tool.hpp>
#include <lxsdk/lx_toolui.hpp>
#include <lxsdk/lx_layer.hpp>
#include <lxsdk/lx_vector.hpp>
#include <lxsdk/lx_pmodel.hpp>

#include "subdivide.hpp"

using namespace lx_err;

#define SRVNAME_TOOL   "tool.osd"
#define SRVNAME_TOOLOP "toolop.osd"

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

//
// Incremental update data for the fast path
//
struct CIncremental
{
    CIncremental(CSubdivide* csub)
    {
        m_csub = csub;
    }
    CSubdivide* m_csub;
};


//
// The Tool Operation is evaluated by the procedural modeling system.
//
class CToolOp : public CLxImpl_ToolOperation, public CLxImpl_MeshElementGroup
{
	public:
        ~CToolOp ()
        {
            for(auto& inc : m_incrementals)
            {
                if (inc.m_csub)
                    delete inc.m_csub;
            }
        }
    
        // ToolOperation Interface
		LxResult    top_Evaluate(ILxUnknownID vts)  LXx_OVERRIDE;
		LxResult    top_ReEvaluate(ILxUnknownID vts)  LXx_OVERRIDE;
    
        // MeshElementGroup Interface
        LxResult	eltgrp_GroupCount	(unsigned int *count)				LXx_OVERRIDE;
        LxResult	eltgrp_GroupName	(unsigned int index, const char **name)		LXx_OVERRIDE;
        LxResult	eltgrp_GroupUserName	(unsigned int index, const char **username)	LXx_OVERRIDE;
        LxResult	eltgrp_TestPolygon	(unsigned int index, LXtPolygonID polygon)	LXx_OVERRIDE;
        LxResult    eltgrp_TestEdge(unsigned int index, LXtEdgeID edge)	LXx_OVERRIDE;
        LxResult    eltgrp_TestPoint(unsigned int index, LXtPointID point)	LXx_OVERRIDE;

        CLxUser_Subject2Packet subject;

        unsigned offset_subject;

        int m_level;
        int m_scheme;
        int m_boundary;
        int m_fvar;
        int m_crease;
        int m_triangle;

        std::vector<CIncremental> m_incrementals;
};

/*
 * OpenSubdiv subdivision tool operator. Basic tool and tool model methods are defined here. The
 * attributes interface is inherited from the utility class.
 */

class CTool : public CLxImpl_Tool, public CLxImpl_ToolModel, public CLxDynamicAttributes
{
public:
    CTool();

    void        tool_Reset() LXx_OVERRIDE;
    LXtObjectID tool_VectorType() LXx_OVERRIDE;
    const char* tool_Order() LXx_OVERRIDE;
    LXtID4      tool_Task() LXx_OVERRIDE;
	LxResult	tool_GetOp(void **ppvObj, unsigned flags) LXx_OVERRIDE;
    unsigned    tool_CompareOp(ILxUnknownID vts, ILxUnknownID other_obj) LXx_OVERRIDE;
    LxResult    tool_UpdateOp(ILxUnknownID other_obj) LXx_OVERRIDE;

    LxResult    tmod_Enable(ILxUnknownID obj) LXx_OVERRIDE;

    using CLxDynamicAttributes::atrui_UIHints;  // to distinguish from the overloaded version in CLxImpl_AttributesUI

    void atrui_UIHints2(unsigned int index, CLxUser_UIHints& hints) LXx_OVERRIDE;

    bool TestPolygon();

    CLxUser_LogService   s_log;
    CLxUser_LayerService s_layer;
    CLxUser_VectorType   v_type;
    CLxUser_SelectionService s_sel;

    unsigned offset_subject;

    static void initialize()
    {
        CLxGenericPolymorph* srv;

        srv = new CLxPolymorph<CTool>;
        srv->AddInterface(new CLxIfc_Tool<CTool>);
        srv->AddInterface(new CLxIfc_ToolModel<CTool>);
        srv->AddInterface(new CLxIfc_Attributes<CTool>);
        srv->AddInterface(new CLxIfc_AttributesUI<CTool>);
        srv->AddInterface(new CLxIfc_StaticDesc<CTool>);
        thisModule.AddServer(SRVNAME_TOOL, srv);

        srv = new CLxPolymorph<CToolOp>;
        srv->AddInterface(new CLxIfc_ToolOperation<CToolOp>);
	    srv->AddInterface(new CLxIfc_MeshElementGroup<CToolOp>);
        lx::AddSpawner(SRVNAME_TOOLOP, srv);
    }

    static LXtTagInfoDesc descInfo[];
};

