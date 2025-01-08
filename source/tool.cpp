//
// OpenSubdiv mesh modifier named "tool.osd.item"
//

#include "subdivide.hpp"
#include "tool.hpp"

//
// On create we add our one tool attribute. We also allocate a vector type
// and select mode mask.
//
CTool::CTool()
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

    CLxUser_PacketService sPkt;
    CLxUser_MeshService   sMesh;

    dyna_Add(ARGs_LEVEL, LXsTYPE_INTEGER);

    dyna_Add(ARGs_SCHEME, LXsTYPE_INTEGER);
    dyna_SetHint(ARGi_SCHEME, subdivide_scheme);

    dyna_Add(ARGs_BOUNDARY, LXsTYPE_INTEGER);
    dyna_SetHint(ARGi_BOUNDARY, subdivide_boundary);

    dyna_Add(ARGs_FVAR, LXsTYPE_INTEGER);
    dyna_SetHint(ARGi_FVAR, subdivide_fvar);

    dyna_Add(ARGs_CREASE, LXsTYPE_INTEGER);
    dyna_SetHint(ARGi_CREASE, subdivide_crease);

    dyna_Add(ARGs_TRIANGLE, LXsTYPE_INTEGER);
    dyna_SetHint(ARGi_TRIANGLE, triangle_subdivision);

    tool_Reset();

    sPkt.NewVectorType(LXsCATEGORY_TOOL, v_type);
    sPkt.AddPacket(v_type, LXsP_TOOL_SUBJECT2, LXfVT_GET);

    offset_subject = sPkt.GetOffset(LXsCATEGORY_TOOL, LXsP_TOOL_SUBJECT2);
}

//
// Reset sets the attributes back to defaults.
//
void CTool::tool_Reset()
{
    dyna_Value(ARGi_LEVEL).SetInt(2);
    dyna_Value(ARGi_SCHEME).SetInt(1);
    dyna_Value(ARGi_BOUNDARY).SetInt(0);
    dyna_Value(ARGi_FVAR).SetInt(0);
    dyna_Value(ARGi_CREASE).SetInt(1);
    dyna_Value(ARGi_TRIANGLE).SetInt(1);
}

LXtObjectID CTool::tool_VectorType()
{
    return v_type.m_loc;  // peek method; does not add-ref
}

const char* CTool::tool_Order()
{
    return LXs_ORD_ACTR;
}

LXtID4 CTool::tool_Task()
{
    return LXi_TASK_ACTR;
}

LxResult CTool::tool_GetOp(void** ppvObj, unsigned flags)
{
    CLxSpawner<CToolOp> spawner(SRVNAME_TOOLOP);
    CToolOp*            toolop = spawner.Alloc(ppvObj);

	if (!toolop)
	{
		return LXe_FAILED;
	}

    dyna_Value(ARGi_LEVEL).GetInt(&toolop->m_level);
    dyna_Value(ARGi_SCHEME).GetInt(&toolop->m_scheme);
    dyna_Value(ARGi_BOUNDARY).GetInt(&toolop->m_boundary);
    dyna_Value(ARGi_FVAR).GetInt(&toolop->m_fvar);
    dyna_Value(ARGi_CREASE).GetInt(&toolop->m_crease);
    dyna_Value(ARGi_TRIANGLE).GetInt(&toolop->m_triangle);

    toolop->offset_subject = offset_subject;

	return LXe_OK;
}

unsigned CTool::tool_CompareOp(ILxUnknownID	vts, ILxUnknownID other_obj)
{
    CToolOp*   prevOp = nullptr;

    lx::CastServer (SRVNAME_TOOLOP, other_obj, prevOp);

    if (prevOp)
    {
        int level, scheme, fvar;
        dyna_Value(ARGi_LEVEL).GetInt(&level);
        dyna_Value(ARGi_SCHEME).GetInt(&scheme);
        dyna_Value(ARGi_FVAR).GetInt(&fvar);
        if ((prevOp->m_level == level) &&
            (prevOp->m_scheme == scheme) &&
            (prevOp->m_fvar == fvar))
        {
            return LXiTOOLOP_COMPATIBLE;
        }
    }

	return LXiTOOLOP_DIFFERENT;
}

LxResult CTool::tool_UpdateOp(ILxUnknownID other_obj)
{
    CToolOp*   prevOp = nullptr;

    lx::CastServer (SRVNAME_TOOLOP, other_obj, prevOp);

    if (prevOp)
    {
        dyna_Value(ARGi_BOUNDARY).GetInt(&prevOp->m_boundary);
        dyna_Value(ARGi_CREASE).GetInt(&prevOp->m_crease);
        dyna_Value(ARGi_TRIANGLE).GetInt(&prevOp->m_triangle);
    }

	return LXe_OK;
}

LXtTagInfoDesc CTool::descInfo[] =
{
	{LXsTOOL_PMODEL, "."},
	{LXsTOOL_USETOOLOP, "."},
	{LXsPMODEL_SELECTIONTYPES, LXsSELOP_TYPE_POLYGON},
    {LXsPMODEL_NOTRANSFORM, "."},
	{0}

};

LxResult CTool::tmod_Enable(ILxUnknownID obj)
{
    CLxUser_Message msg(obj);

    if (TestPolygon() == false)
    {
        msg.SetCode(LXe_CMD_DISABLED);
        msg.SetMessage(SRVNAME_TOOL, "NoPolygon", 0);
        return LXe_DISABLED;
    }
    return LXe_OK;
}

void CTool::atrui_UIHints2(unsigned int index, CLxUser_UIHints& hints)
{
    switch (index)
    {
        case ARGi_LEVEL:
            hints.MinInt(0);
            break;
    }
}

bool CTool::TestPolygon()
{
    //
    // Start the scan in read-only mode.
    //
    CLxUser_LayerScan scan;
    CLxUser_Mesh      mesh;
    unsigned          i, n, count;
    bool              ok = false;

    s_layer.BeginScan(LXf_LAYERSCAN_ACTIVE | LXf_LAYERSCAN_MARKPOLYS, scan);

    //
    // Count the polygons in all mesh layers.
    //
    if (scan)
    {
        n = scan.NumLayers();
        for (i = 0; i < n; i++)
        {
            scan.BaseMeshByIndex(i, mesh);
            mesh.PolygonCount(&count);
            if (count > 0)
            {
                ok = true;
                break;
            }
        }
        scan.Apply();
    }

    //
    // Return false if there is no polygons in any active layers.
    //
    return ok;
}

//
// Tool evaluation uses layer scan interface to walk through all the active
// meshes and visit all the selected polygons.
//
LxResult CToolOp::top_Evaluate(ILxUnknownID vts)
{
    CLxUser_VectorStack vec(vts);

    printf("top_Evaluate\n");
    //
    // Start the scan in edit mode.
    //
    CLxUser_LayerScan  scan;
    CLxUser_Mesh       base_mesh, edit_mesh;

    if (vec.ReadObject(offset_subject, subject) == false)
        return LXe_FAILED;

    CLxUser_MeshService   s_mesh;

    LXtMarkMode pick = s_mesh.SetMode(LXsMARK_SELECT);

    subject.BeginScan(LXf_LAYERSCAN_EDIT_POLVRT, scan);

    auto n = scan.NumLayers();
    for (auto i = 0u; i < n; i++)
    {
        scan.BaseMeshByIndex(i, base_mesh);
        scan.EditMeshByIndex(i, edit_mesh);

        CSubdivide* csub = new CSubdivide;

        csub->SetLevel(m_level);
        csub->SetScheme(m_scheme);
        csub->SetBoundary(m_boundary);
        csub->SetFVar(m_fvar);
        csub->SetCrease(m_crease);
        csub->SetTriangle(m_triangle);

        if (csub->Build(base_mesh, pick))
        {
            if (csub->Apply(edit_mesh))
            {
                CIncremental inc (csub);
                m_incrementals.push_back(inc);
            }
        }

        scan.SetMeshChange(i, LXf_MESHEDIT_GEOMETRY);
    }

    scan.Apply();
    return LXe_OK;
}

//
// Refine the souce cage positions and update the new subdivided vertices with
// the refined positions.
//
LxResult CToolOp::top_ReEvaluate(ILxUnknownID vts)
{
    printf("top_ReEvaluate increments = %zu\n", m_incrementals.size());
    printf("-- boundary %d fvar %d triangle %d crease %d\n", m_boundary, m_fvar, m_triangle, m_crease);
    for (auto& inc : m_incrementals)
    {
        inc.m_csub->SetBoundary(m_boundary);
        inc.m_csub->SetCrease(m_crease);
        inc.m_csub->SetTriangle(m_triangle);

        if (inc.m_csub->Refine())
        {
            inc.m_csub->Update();
        }
    }
    return LXe_OK;
}

