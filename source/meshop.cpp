//
// OpenSubdiv mesh modifier named "mesh.osd.item"
//

#include "meshop.hpp"
#include "subdivide.hpp"

LXxC_MeshOperation_Evaluate(CMeshOp)  // (ILxUnknownID mesh, LXtID4 type, LXtMarkMode mode)
{
    CLxUser_Mesh        edit_mesh;

    edit_mesh.set(mesh);

    if (!edit_mesh.test() || type != LXiSEL_POLYGON)
        return LXe_OK;

    CSubdivide csub;
    int ival;

    attr_GetInt(ARGi_MESHOP_LEVEL, &ival);
    csub.m_level = static_cast<unsigned>(ival);

    attr_GetInt(ARGi_MESHOP_SCHEME, &ival);
    csub.m_scheme = static_cast<Sdc::SchemeType>(ival);

    attr_GetInt(ARGi_MESHOP_ADAPTIVE, &ival);
    csub.m_adaptive = static_cast<bool>(ival);

    attr_GetInt(ARGi_MESHOP_BOUNDARY, &ival);
    csub.m_boundary = static_cast<Sdc::Options::VtxBoundaryInterpolation>(ival);

    attr_GetInt(ARGi_MESHOP_FVAR, &ival);
    csub.m_fvar = static_cast<Sdc::Options::FVarLinearInterpolation>(ival);

    attr_GetInt(ARGi_MESHOP_TRIANGLE, &ival);
    csub.m_triangle = static_cast<Sdc::Options::TriangleSubdivision>(ival);

    csub.Build(edit_mesh);
    csub.Apply(edit_mesh);

    edit_mesh.SetMeshEdits(LXf_MESHEDIT_GEOMETRY);
    return LXe_OK;
}

LXtTagInfoDesc CMeshOp::descInfo[] = { { LXsMESHOP_PMODEL, "." },
                                        { LXsPMODEL_SELECTIONTYPES, LXsSELOP_TYPE_POLYGON },
                                        { LXsPMODEL_NOTRANSFORM, "." },
                                        { nullptr } };
