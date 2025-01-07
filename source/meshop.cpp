//
// OpenSubdiv mesh modifier named "mesh.osd.item"
//

#include "meshop.hpp"
#include "subdivide.hpp"

LXxC_MeshOperation_Evaluate(CMeshOp)  // (ILxUnknownID mesh, LXtID4 type, LXtMarkMode mode)
{
    CLxUser_Mesh        edit_mesh;

    printf("** Evaluate **\n");
    edit_mesh.set(mesh);

    if (!edit_mesh.test() || type != LXiSEL_POLYGON)
        return LXe_OK;

    int ival;

    dyna_Value(ARGi_LEVEL).GetInt(&ival);
    m_csub.SetLevel (ival);

    dyna_Value(ARGi_SCHEME).GetInt(&ival);
    m_csub.SetScheme (ival);

    dyna_Value(ARGi_BOUNDARY).GetInt(&ival);
    m_csub.SetBoundary (ival);

    dyna_Value(ARGi_FVAR).GetInt(&ival);
    m_csub.SetFVar (ival);

    dyna_Value(ARGi_TRIANGLE).GetInt(&ival);
    m_csub.SetTriangle (ival);

    if (LXx_OK (edit_mesh.BeginEditBatch ()))
    {
        if (m_csub.Build(edit_mesh, LXiMARK_ANY) == false)
            return LXe_FAILED;

        if (m_csub.Apply(edit_mesh) == false)
            return LXe_FAILED;

        edit_mesh.SetMeshEdits(LXf_MESHEDIT_GEOMETRY);
        edit_mesh.EndEditBatch ();

        return LXe_OK;
    }

    return LXe_FAILED;
}

LXxC_MeshOperation_ReEvaluate(CMeshOp)  // (ILxUnknownID mesh, LXtID4 type)
{
    CLxUser_Mesh        edit_mesh;

    printf("** ReEvaluate **\n");
    edit_mesh.set(mesh);

    if (!edit_mesh.test() || type != LXiSEL_POLYGON)
        return LXe_OK;

    int ival;

    dyna_Value(ARGi_LEVEL).GetInt(&ival);
    m_csub.SetLevel (ival);

    dyna_Value(ARGi_SCHEME).GetInt(&ival);
    m_csub.SetScheme (ival);

    dyna_Value(ARGi_BOUNDARY).GetInt(&ival);
    m_csub.SetBoundary (ival);

    dyna_Value(ARGi_FVAR).GetInt(&ival);
    m_csub.SetFVar (ival);

    dyna_Value(ARGi_TRIANGLE).GetInt(&ival);
    m_csub.SetTriangle (ival);

    if (m_csub.Refine() == false)
        return LXe_FAILED;

    if (m_csub.Update() == false)
        return LXe_FAILED;

    edit_mesh.SetMeshEdits(LXf_MESHEDIT_POSITION);
    return LXe_OK;
}

LXxC_MeshOperation_Compare(CMeshOp)  // (ILxUnknownID other)
{
    CMeshOp* prevOp = nullptr;

    /*
     * Cast the other interface into our implementation, and
     * then compare the offset attribute.
     */

    lx::CastServer (SRVNAME, other, prevOp);

    if (prevOp)
    {
        int level, scheme, boundary, fvar, triangle;
        dyna_Value(ARGi_LEVEL).GetInt(&level);
        dyna_Value(ARGi_SCHEME).GetInt(&scheme);
        dyna_Value(ARGi_BOUNDARY).GetInt(&boundary);
        dyna_Value(ARGi_FVAR).GetInt(&fvar);
        dyna_Value(ARGi_TRIANGLE).GetInt(&triangle);
        if ((prevOp->m_csub.m_level == level) &&
            (prevOp->m_csub.m_scheme == scheme))
        {
            if ((prevOp->m_csub.m_boundary == boundary) &&
                (prevOp->m_csub.m_fvar == fvar) &&
                (prevOp->m_csub.m_triangle == triangle))
            {
                printf("-- LXiMESHOP_IDENTICAL --\n");
                return LXiMESHOP_IDENTICAL;
            }
            printf("-- LXiMESHOP_COMPATIBLE --\n");
            return LXiMESHOP_COMPATIBLE;
        }
    }
    printf("-- LXiMESHOP_DIFFERENT --\n");
    return LXiMESHOP_DIFFERENT;
}

LXxC_MeshOperation_Convert(CMeshOp)  // (ILxUnknownID other)
{
    CMeshOp* prevOp = nullptr;

    /*
     * Cast the other interface into our implementation, and
     * then copy the cached points that want to offset.
     */

    lx::CastServer (SRVNAME, other, prevOp);

    if (prevOp)
    {
        m_csub = prevOp->m_csub;
        return LXe_OK;
    }
    return LXe_FAILED;
}

LXxC_MeshOperation_SetTransform(CMeshOp)
{
    return LXe_OK;
}

LXtTagInfoDesc CMeshOp::descInfo[] = { { LXsMESHOP_PMODEL, "." },
                                        { LXsPMODEL_SELECTIONTYPES, LXsSELOP_TYPE_POLYGON },
                                        { nullptr } };
