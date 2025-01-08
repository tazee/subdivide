//
// Subdivide class to wrap OpenSubdiv library.
//

#include "subdivide.hpp"

//
// Build opensubdiv refiner and generates subdivided positions.
//
bool CSubdivide::Build (CLxUser_Mesh& mesh, LXtMarkMode pick)
{
    printf("CSubdivide::Build\n");
	m_mesh.set (mesh);
    m_pick = pick;

    m_points.clear();
    m_polygons.clear();

    // Clear the previous data.
    Clear();

    // Get the cage polygons and their vertices.
    if (SetupCages() == false)
        return false;

    // Refine the subdivided positions from the source mesh vertices.
    return Refine();
}


//
// Clear all context
//
void CSubdivide::Clear ()
{
    m_points.clear ();
    m_polygons.clear ();
    m_fvarArray.clear ();
    m_subdiv_positions.clear ();
    m_subdiv_fvars.clear ();
    m_subdiv_points.clear ();
    if (m_refiner) {
        delete m_refiner;
        m_refiner = nullptr;
    }   
}

//
// Refine subdivided positions from source mesh vertices.
//
bool CSubdivide::Refine ()
{
    printf("CSubdivide::Refine mesh (%d)\n", m_mesh.test ());
	if (!m_mesh.test ())
		return false;

	CLxUser_Point   upoint;
	LXtFVector	    pos;

	m_mesh.GetPoints (upoint);
	auto nvrt = m_points.size ();
	if (!nvrt)
		return false;

    // Create a new topology refiner.
    if (!m_refiner)
    {
        m_refiner = CreateTopologyRefiner();
        if (!m_refiner)
            return false;
    }

    // Allocate position buffer for subdiv positions in all levels and setup the source point positions.
    m_subdiv_positions.resize(m_refiner->GetNumVerticesTotal());
    for (auto i = 0u; i < nvrt; i++) {
        upoint.Select (m_points[i]);
        upoint.Pos (pos);
        m_subdiv_positions[i].SetPoint (pos[0], pos[1], pos[2]);
    }

    // Allocate UV buffer for subdiv positions in all levels and setup the source UV values.
    m_subdiv_fvars.resize(m_fvarArray.size());
    for (auto i = 0u; i < m_fvarArray.size(); i++)
    {
        m_subdiv_fvars[i].resize(m_refiner->GetNumFVarValuesTotal(i));
        for (auto j = 0u; j < m_fvarArray[i].values.size() / 2; j++)
        {
            m_subdiv_fvars[i][j]._value[0] = m_fvarArray[i].values[j * 2];
            m_subdiv_fvars[i][j]._value[1] = m_fvarArray[i].values[j * 2 + 1];
        }
    }

    // Interpolate vertex primvar data
    Far::PrimvarRefiner primvarRefiner(*m_refiner);

    Point3 * src = &m_subdiv_positions[0];
    for (auto level = 1; level <= m_level; level++) {
        Point3 * dst = src + m_refiner->GetLevel(level-1).GetNumVertices();
        primvarRefiner.Interpolate(level, src, dst);
        src = dst;
    }

    // Interpolate vertex map values
    for (auto i = 0u; i < m_subdiv_fvars.size(); i++)
    {
        FVarValue* srcFVar = &m_subdiv_fvars[i][0];
        for (auto level = 1; level <= m_level; level++) {
            FVarValue * dstFVar = srcFVar + m_refiner->GetLevel(level-1).GetNumFVarValues(i);
            primvarRefiner.InterpolateFaceVarying(level, srcFVar, dstFVar, i);
            srcFVar = dstFVar;
        }
    }

	return true;
}

//
// Apply the subdivided positions to the edit mesh.
//
bool CSubdivide::Apply (CLxUser_Mesh& edit_mesh)
{
    printf("CSubdivide::Apply\n");
    if (!m_polygons.size() || !m_points.size())
        return false;

    m_edit_mesh.set(edit_mesh);

    std::vector<LXtPolygonID> polygons;

    CreateSubdivPoints(m_subdiv_points);
    CreateSubdivPolygons(m_subdiv_points, polygons);
    CreateSubdivFVars(m_subdiv_points, polygons);
    RemoveSourcePolygons();
    RemoveSourcePoints();

	return true;
}

//
// Update the edit mesh with the subdivided positions.
//
bool CSubdivide::Update()
{
    printf("CSubdivide::Update = %zu\n", m_subdiv_positions.size());
    if (!m_polygons.size() || !m_points.size())
        return false;

    LXtVector       pos;
    CLxUser_Point   upoint;
    m_edit_mesh.GetPoints(upoint);

    int offset = 0;
    for (auto level = 0; level < m_level; level++) {
        offset += m_refiner->GetLevel(level).GetNumVertices();
    }

    for (auto i = 0u; i < m_subdiv_points.size(); i++) 
    {
        const float * f = m_subdiv_positions[i + offset].GetPoint();
        LXx_VCPY(pos, f);
        upoint.Select(m_subdiv_points[i]);
        upoint.SetPos(pos);
    }

    return true;
}

//
// Create a topology refiner for the subdivided positions.
//
bool CSubdivide::CreateSubdivPoints(std::vector<LXtPointID>& points)
{
    LXtVector       pos;
    LXtPointID      pnt;
    CLxUser_Point   upoint;
    m_edit_mesh.GetPoints(upoint);

    int offset = 0;
    for (auto level = 0; level < m_level; level++) {
        offset += m_refiner->GetLevel(level).GetNumVertices();
    }

    int nverts = m_refiner->GetLevel(m_level).GetNumVertices();
    points.resize(nverts);

    for (auto i = 0; i < nverts; i++) 
    {
        const float * f = m_subdiv_positions[i + offset].GetPoint();
        LXx_VCPY(pos, f);
        upoint.New(pos, &pnt);
        points[i] = pnt;
    }

    return true;
}

//
// Create subdivided polygons from the source mesh.
//
bool CSubdivide::CreateSubdivPolygons(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons)
{
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);
    auto numFaces = refLastLevel.GetNumFaces();
    LXtPointID       verts[4];
    LXtPolygonID     polID;
    LXtID4           type = LXiPTYP_FACE;

    CLxUser_Polygon  upoly;
    m_edit_mesh.GetPolygons(upoly);

    polygons.clear();

    for (auto face = 0; face < numFaces; face++)
    {
        Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);
        for (int i = 0; i < fverts.size(); i++) {
            auto j = static_cast<int>(fverts[i]);
            verts[i] = points[j];
        }
        GetCagePolygon(face, upoly);
        upoly.NewProto(type, verts, fverts.size(), 0, &polID);
        polygons.push_back(polID);
    }
    return true;
}

//
// Create subdivided vertex map values from the source mesh.
//
bool CSubdivide::CreateSubdivFVars(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons)
{
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);
    auto numFaces = refLastLevel.GetNumFaces();
    LXtPointID       pnt;
    float            value[2];

    CLxUser_Polygon  upoly;
    m_edit_mesh.GetPolygons(upoly);

	CLxUser_MeshMap	 umap;
	m_edit_mesh.GetMaps(umap);

    for (auto iv = 0u; iv < m_fvarArray.size(); iv++)
    {
        umap.SelectByName (LXi_VMAP_TEXTUREUV, m_fvarArray[iv].name.c_str());
        LXtMeshMapID     mapID = umap.ID();
        int offset = 0;
        for (auto level = 0; level < m_level; level++) {
            offset += m_refiner->GetLevel(level).GetNumFVarValues(iv);
        }
        for (auto face = 0; face < numFaces; face++)
        {
            Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);
            Far::ConstIndexArray fuvs   = refLastLevel.GetFaceFVarValues(face, iv);
            upoly.Select(polygons[face]);
            for (int i = 0; i < fverts.size(); i++) {
                auto j = static_cast<int>(fverts[i]);
                assert((j >= 0) && (j < points.size()));
                pnt = points[j];
                auto k = static_cast<int>(fuvs[i]);
                assert(pnt != nullptr);
                value[0] = m_subdiv_fvars[iv][k + offset]._value[0];
                value[1] = m_subdiv_fvars[iv][k + offset]._value[1];
				upoly.SetMapValue (pnt, mapID, value);
            }
        }
    }
    return true;
}

//
// Get the cage polygon at level 0 for the given subdivided face.
//
int CSubdivide::GetCagePolygon(int face, CLxUser_Polygon& upoly)
{
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);
    int                       parent       = refLastLevel.GetFaceParentFace(face);
    unsigned int              level        = m_level;
    while (--level)
    {
        Far::TopologyLevel const& refLevel = m_refiner->GetLevel(level);
        parent                             = refLevel.GetFaceParentFace(parent);
    }

    upoly.SelectByIndex(parent);
    return parent;
}

//
// Setup cages polygons and their vertices from the source mesh.
//
bool CSubdivide::SetupCages()
{
	if (!m_mesh.test ())
		return false;

    m_polygons.clear();
    m_points.clear();
    m_polyFaceMap.clear();
    m_fvarArray.clear();

    CLxUser_Polygon upoly;
    m_mesh.GetPolygons(upoly);

    CLxUser_MeshService mS;

	//
	// Collect polygons and their vertices from the source mesh.
	//
    PolygonMeshBinVisitor vis(upoly, m_mbin, m_polygons);
    m_mbin.npols          = 0;
    m_mbin.polyVertCounts = 0;
    vis.m_mask = m_pick;
    vis.m_upoly.Enum(&vis);
    if (m_mbin.npols == 0)
        return false;

	//
	// Create a map of polygon IDs to their indices.
	//
    for (auto i = 0; i < m_polygons.size(); i++)
        m_polyFaceMap[m_polygons[i]] = i;

	//
	// Mark the vertices of the polygons to be subdivided.
	//
    VertexMarkVisitor mark;

    m_mesh.GetPolygons(mark.m_poly);
    m_mesh.GetPoints(mark.m_point);
    mark.m_clear = true;
    mark.m_mask  = mS.ClearMode(LXsMARK_USER_1);
    mark.m_point.Enum(&mark);
    mark.m_clear = false;
    mark.m_mask  = mS.SetMode(LXsMARK_USER_1);
    mark.m_poly.Enum(&mark);

	//
	// Collect the vertices of the marked polygons.
	//
    VertexTableVisitor vtab(m_points);

    m_mesh.GetPoints(vtab.m_point);
    vtab.m_mask = mS.SetMode(LXsMARK_USER_1);
    vtab.m_point.Enum(&vtab);
    if (m_points.size() == 0)
        return false;

    return true;
}

//
// Remove the source polygons.
//
void CSubdivide::RemoveSourcePolygons()
{
    CLxUser_Polygon upoly;

    m_edit_mesh.GetPolygons(upoly);
    for (auto i = 0u; i < m_polygons.size(); i++)
    {
        upoly.Select(m_polygons[i]);
        upoly.Remove();
    }
}

//
// Remove the source vertices.
//
void CSubdivide::RemoveSourcePoints()
{
    CLxUser_Point upoint;
    LXtPolygonID  polID;

    CLxUser_MeshService mS;
    VertexMarkVisitor   mark;

    m_mesh.GetPolygons(mark.m_poly);
    m_mesh.GetPoints(mark.m_point);
    mark.m_clear = true;
    mark.m_mask  = mS.ClearMode(LXsMARK_USER_1);
    mark.m_poly.Enum(&mark);

    mark.m_mask  = mS.SetMode(LXsMARK_USER_1);
    for (auto i = 0u; i < m_polygons.size(); i++)
    {
        mark.m_poly.Select(m_polygons[i]);
        mark.m_poly.SetMarks(mark.m_mask);
    }

    m_edit_mesh.GetPoints(upoint);
    unsigned int count;
    for (auto i = 0u; i < m_points.size(); i++)
    {
        upoint.Select(m_points[i]);
        upoint.PolygonCount(&count);
        bool ok = true;
        for (auto j = 0u; j < count; j++)
        {
            upoint.PolygonByIndex(j, &polID);
            mark.m_poly.Select(polID);
            // Don't remove the vertex if it is used for other polygon.
            if (mark.m_poly.TestMarks(mark.m_pick) == LXe_FALSE)
            {
                ok = false;
                break;
            }
        }
        if (ok)
            upoint.Remove();
    }
}


//
// Create a topology refiner from cage polygons of source mesh.
//
Far::TopologyRefiner* CSubdivide::CreateTopologyRefiner()
{
    if (!m_mesh.test())
        return nullptr;

    //
    // Setup a topology descriptor from source polygons.
    //
    Descriptor desc{};

    CLxUser_Point upoint;
    m_mesh.GetPoints(upoint);
    desc.numVertices = static_cast<int>(m_points.size());
    if (!desc.numVertices)
        return nullptr;

    int npol = m_mesh.NPolygons();
    if (!npol)
        return nullptr;

    //
    // Count number of subdiv polygons and total indices.
    //
    CLxUser_Polygon upoly;
    unsigned int    count, ntri;

    m_mesh.GetPolygons(upoly);
    desc.numFaces = 0;

	desc.numFaces = m_mbin.npols;
	auto faceVertCounts = m_mbin.polyVertCounts;
    if (m_scheme == Sdc::SCHEME_LOOP)
    {
	    desc.numFaces  = m_mbin.ntris;
        faceVertCounts = m_mbin.triVertCounts;
    }

    //
    // Store the number of face vertex and vertex indices.
    //
    std::vector<int> vertsPerFace;
    std::vector<int> vertIndices;

    vertsPerFace.resize(desc.numFaces);
    vertIndices.resize(faceVertCounts);

    LXtPointID point, point1, point2, point3;

    std::map<LXtPointID, int> pointIndexMap;

    for (auto i = 0u; i < m_points.size(); i++)
        pointIndexMap[m_points[i]] = i;

	auto m = 0u, n = 0u;
    for (auto i = 0u; i < m_polygons.size(); i++)
    {
        upoly.Select(m_polygons[i]);
        upoly.VertexCount(&count);
        // triagulate polygons for loop scheme
        if (m_scheme == Sdc::SCHEME_LOOP)
        {
            upoly.GenerateTriangles(&ntri);
            for (auto j = 0u; j < ntri; j++)
            {
                upoly.TriangleByIndex(j, &point1, &point2, &point3);
                vertsPerFace[n++] = 3;
                vertIndices[m++] = pointIndexMap[point1];
                vertIndices[m++] = pointIndexMap[point2];
                vertIndices[m++] = pointIndexMap[point3];
            }
        }
        else
        {
            vertsPerFace[n++] = static_cast<int>(count);
            for (auto j = 0u; j < count; j++)
            {
                upoly.VertexByIndex(j, &point);
                vertIndices[m++] = pointIndexMap[point];
            }
        }
    }

    desc.numVertsPerFace    = &vertsPerFace[0];
    desc.vertIndicesPerFace = &vertIndices[0];

    //
    // Edge creases/ vertex creases
    //
    std::vector<float> edgeCreases;
    std::vector<int>   edgeCreaseIndices;
    std::vector<float> vtxCreases;
    std::vector<int>   vtxCreaseIndices;

    CLxUser_Edge   uedge;
    CLxLoc_MeshMap umap;
    LxResult       res;

    m_mesh.GetMaps(umap);
    res = umap.SelectByName(LXi_VMAP_SUBDIV, "Subdivision");
    if (res == LXe_OK)
    {
        m_mesh.GetEdges(uedge);
        EdgeCreaseVisitor visEdge(umap, upoint, uedge, edgeCreases, edgeCreaseIndices, pointIndexMap);
        visEdge.m_edge.Enum(&visEdge);
        VertexCreaseVisitor visVtx(umap, upoint, vtxCreases, vtxCreaseIndices, pointIndexMap);
        visVtx.m_point.Enum(&visVtx);
        desc.numCreases             = static_cast<int>(edgeCreases.size());
        desc.creaseVertexIndexPairs = &edgeCreaseIndices[0];
        desc.creaseWeights          = &edgeCreases[0];
        desc.numCorners             = static_cast<int>(vtxCreases.size());
        desc.cornerVertexIndices    = &vtxCreaseIndices[0];
        desc.cornerWeights          = &vtxCreases[0];
    }

    //
    // UV maps
    //
    PolygonFVarVisitor visMap(m_mesh, m_fvarArray, m_polygons);
    visMap.m_triangle = (m_scheme == Sdc::SCHEME_LOOP);
    visMap.m_maps.Enum(&visMap);

    desc.numFVarChannels = static_cast<int>(m_fvarArray.size());
    if (desc.numFVarChannels > 0)
    {
        Descriptor::FVarChannel* fvarChannels = new Descriptor::FVarChannel[desc.numFVarChannels];
        for (auto i = 0u; i < m_fvarArray.size(); i++)
        {
            fvarChannels[i].numValues    = static_cast<int>(m_fvarArray[i].indices.size());
            fvarChannels[i].valueIndices = &m_fvarArray[i].indices[0];
        }
        desc.fvarChannels = fvarChannels;
    }

    //
    // Create an OpenSubdiv topology refiner.
    //
    Options options;
    options.SetVtxBoundaryInterpolation(m_boundary);
    options.SetFVarLinearInterpolation(m_fvar);

    Far::TopologyRefiner* refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc, Far::TopologyRefinerFactory<Descriptor>::Options(m_scheme, options));
    if (desc.numFVarChannels > 0)
        delete[] desc.fvarChannels;

    //
    // Refine the refiner by uniform at the given level.
    //
    refiner->RefineUniform(Far::TopologyRefiner::UniformOptions(m_level));
    return refiner;
}

//
// Accessors to set attributes.
//
void CSubdivide::SetLevel (int level)
{
    if (m_level == level)
        return;
    m_level = static_cast<unsigned>(level);
    if (m_refiner)
    {
        delete m_refiner;
        m_refiner = nullptr;
    }
}

void CSubdivide::SetScheme (int scheme)
{
    if (m_scheme == scheme)
        return;
    m_scheme = static_cast<Sdc::SchemeType>(scheme);
    if (m_refiner)
    {
        delete m_refiner;
        m_refiner = nullptr;
    }
}

void CSubdivide::SetBoundary (int bounary)
{
    if (m_boundary == bounary)
        return;
    m_boundary = static_cast<Sdc::Options::VtxBoundaryInterpolation>(bounary);
    if (m_refiner)
    {
        delete m_refiner;
        m_refiner = nullptr;
    }
}

void CSubdivide::SetFVar (int fvar)
{
    if (m_fvar == fvar)
        return;
    m_fvar = static_cast<Sdc::Options::FVarLinearInterpolation>(fvar);
    if (m_refiner)
    {
        delete m_refiner;
        m_refiner = nullptr;
    }
}

void CSubdivide::SetCrease (int crease)
{
    if (m_crease == crease)
        return;
    m_crease = static_cast<Sdc::Options::CreasingMethod>(crease);
    if (m_refiner)
    {
        delete m_refiner;
        m_refiner = nullptr;
    }
}

void CSubdivide::SetTriangle(int triangle)
{
    if (m_triangle == triangle)
        return;
    m_triangle = static_cast<Sdc::Options::TriangleSubdivision>(triangle);
    if (m_refiner)
    {
        delete m_refiner;
        m_refiner = nullptr;
    }
}