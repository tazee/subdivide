//
// The command version of subdivision using OpenSubdiv.
//
#include "command.hpp"

void CCommand::basic_Execute(unsigned int /*flags*/)
{
    CLxUser_LayerScan scan;
    unsigned          n;

    CSubdivide csub;
    int ival;

    attr_GetInt(ARGi_COMMAND_LEVEL, &ival);
    csub.m_level = static_cast<unsigned>(ival);

    attr_GetInt(ARGi_COMMAND_SCHEME, &ival);
    csub.m_scheme = static_cast<Sdc::SchemeType>(ival);

    attr_GetInt(ARGi_COMMAND_ADAPTIVE, &ival);
    csub.m_adaptive = static_cast<bool>(ival);

    attr_GetInt(ARGi_COMMAND_BOUNDARY, &ival);
    csub.m_boundary = static_cast<Sdc::Options::VtxBoundaryInterpolation>(ival);

    attr_GetInt(ARGi_COMMAND_FVAR, &ival);
    csub.m_fvar = static_cast<Sdc::Options::FVarLinearInterpolation>(ival);

    attr_GetInt(ARGi_COMMAND_CREASE, &ival);
    csub.m_crease = static_cast<Sdc::Options::CreasingMethod>(ival);

    attr_GetInt(ARGi_COMMAND_TRIANGLE, &ival);
    csub.m_triangle = static_cast<Sdc::Options::TriangleSubdivision>(ival);

    check(lyr_S.BeginScan(LXf_LAYERSCAN_EDIT_POLYS, scan));
    check(scan.Count(&n));

    CLxUser_Mesh        base_mesh;
    CLxUser_Mesh        edit_mesh;

    for (auto i = 0u; i < n; i++)
    {
        check(scan.BaseMeshByIndex(i, base_mesh));
        check(scan.EditMeshByIndex(i, edit_mesh));

        csub.Build(base_mesh);
        csub.Apply(edit_mesh);

        scan.SetMeshChange(i, LXf_MESHEDIT_GEOMETRY);
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
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_COMMAND_LEVEL)) == false)
    {
        attr_SetInt(ARGi_COMMAND_LEVEL, 1);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_COMMAND_SCHEME)) == false)
    {
        attr_SetInt(ARGi_COMMAND_SCHEME, 1);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_COMMAND_ADAPTIVE)) == false)
    {
        attr_SetInt(ARGi_COMMAND_ADAPTIVE, 0);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_COMMAND_BOUNDARY)) == false)
    {
        attr_SetInt(ARGi_COMMAND_BOUNDARY, 0);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_COMMAND_FVAR)) == false)
    {
        attr_SetInt(ARGi_COMMAND_FVAR, 5);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_COMMAND_CREASE)) == false)
    {
        attr_SetInt(ARGi_COMMAND_CREASE, 0);
    }
    if (LXxCMDARG_ISSET(dyna_GetFlags(ARGi_COMMAND_TRIANGLE)) == false)
    {
        attr_SetInt(ARGi_COMMAND_TRIANGLE, 1);
    }

    return LXe_OK;
}