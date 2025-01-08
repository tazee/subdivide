//
// The command version of polygon subdivision using OpenSubdiv.
//
#include "command.hpp"

void CCommand::basic_Execute(unsigned int /*flags*/)
{
    CLxUser_LayerScan scan;
    unsigned          n;

    CSubdivide csub;
    int ival;

    attr_GetInt(ARGi_LEVEL, &ival);
    csub.SetLevel(ival);

    attr_GetInt(ARGi_SCHEME, &ival);
    csub.SetScheme(ival);

    attr_GetInt(ARGi_BOUNDARY, &ival);
    csub.SetBoundary(ival);

    attr_GetInt(ARGi_FVAR, &ival);
    csub.SetFVar(ival);

    attr_GetInt(ARGi_CREASE, &ival);
    csub.SetCrease(ival);

    attr_GetInt(ARGi_TRIANGLE, &ival);
    csub.SetTriangle(ival);

    check(lyr_S.BeginScan(LXf_LAYERSCAN_EDIT_POLYS, scan));
    check(scan.Count(&n));

    CLxUser_Mesh        base_mesh;
    CLxUser_Mesh        edit_mesh;

    CLxUser_MeshService mS;
    LXtMarkMode pick = mS.SetMode(LXsMARK_SELECT);

    for (auto i = 0u; i < n; i++)
    {
        check(scan.BaseMeshByIndex(i, base_mesh));
        check(scan.EditMeshByIndex(i, edit_mesh));

        if (csub.Build(base_mesh, pick))
        {
            if (csub.Apply(edit_mesh))
            {
                scan.SetMeshChange(i, LXf_MESHEDIT_GEOMETRY);
            }
        }
    }

    scan.Apply();
}

int CCommand::basic_CmdFlags()
{
    return LXfCMD_MODEL | LXfCMD_UNDO;
}

bool CCommand::basic_Enable(CLxUser_Message& /*msg*/)
{
    int      flags = 0;
    unsigned count;

    check(lyr_S.SetScene(0));
    check(lyr_S.Count(&count));

    for (unsigned i = 0; i < count; i++)
    {
        check(lyr_S.Flags(i, &flags));
        if (flags & LXf_LAYERSCAN_ACTIVE)
            return true;
    }

    return false;
}

LxResult CCommand::cmd_DialogInit(void)
{
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_LEVEL)) == false)
    {
        attr_SetInt(ARGi_LEVEL, 1);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_SCHEME)) == false)
    {
        attr_SetInt(ARGi_SCHEME, 1);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_BOUNDARY)) == false)
    {
        attr_SetInt(ARGi_BOUNDARY, 0);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_FVAR)) == false)
    {
        attr_SetInt(ARGi_FVAR, 0);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_CREASE)) == false)
    {
        attr_SetInt(ARGi_CREASE, 1);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_TRIANGLE)) == false)
    {
        attr_SetInt(ARGi_TRIANGLE, 1);
    }

    return LXe_OK;
}