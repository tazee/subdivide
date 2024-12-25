//
// Subdivide class to wrap OpenSubdiv library.
//

#include "subdivide.hpp"

bool CSubdivide::Build (CLxUser_Mesh& mesh)
{
    printf("CSubdivide::Build\n");
	m_mesh.set (mesh);

    // Clear the previous data.
    Clear();

    // Get the cage polygons and their vertices.
    if (SetupCages() == false)
        return false;

    // Create a new topology refiner.
    m_refiner = CreateTopologyRefiner();
    if (!m_refiner)
        return false;

    // Create a new mesh interface.
    m_glmesh = CreateMeshInterface(m_refiner);
    if (!m_glmesh)
        return false;

    // Refine the subdivided positions from the source mesh vertices.
    return Refine();
}


void CSubdivide::Clear ()
{
    printf("CSubdivide::Clear\n");
    m_points.clear ();
    m_polygons.clear ();
    m_fvarArray.clear ();
    if (m_glmesh) {
        delete m_glmesh;
        m_glmesh = nullptr;
    }   
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
    printf("CSubdivide::Refine\n");
	if (!m_mesh.test ())
		return false;

	CLxUser_Point   upoint;
	LXtFVector		pos;

	m_mesh.GetPoints (upoint);
	auto nvrt = m_points.size ();
	if (!nvrt)
		return false;

	std::vector<float> vertex;
	vertex.resize (nvrt * 3);
	for (auto i = 0u; i < nvrt; i++) {
		upoint.Select (m_points[i]);
		upoint.Pos (pos);
		vertex[i * 3    ] = pos[0];
		vertex[i * 3 + 1] = pos[1];
		vertex[i * 3 + 2] = pos[2];
	}

	m_glmesh->UpdateVertexBuffer (&vertex[0], 0, nvrt);
	m_glmesh->Refine();
	m_glmesh->Synchronize();

	return true;
}

bool CSubdivide::Apply (CLxUser_Mesh& edit_mesh)
{
    printf("CSubdivide::Apply\n");
    m_edit_mesh.set(edit_mesh);
    
    std::vector<LXtPointID>   points;
    std::vector<LXtPolygonID> polygons;

    CreateSubdivPoints(points);
    CreateSubdivPolygons(points, polygons);
	return true;
}

bool CSubdivide::CreateSubdivPoints(std::vector<LXtPointID>& points)
{
    printf("CSubdivide::CreateSubdivPoints\n");
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);
    Far::TopologyLevel const& refCageLevel = m_refiner->GetLevel(0);

    unsigned int size   = refLastLevel.GetNumVertices() * 3;
    unsigned int offset = refCageLevel.GetNumVertices() * 3;
    GLuint vbo = m_glmesh->BindVertexBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const float* pos = static_cast<const float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
    pos += offset;
    LXtVector p;
    LXtPointID pnt;
    CLxUser_Point   point;
    point.fromMesh(m_edit_mesh);
    points.resize(refLastLevel.GetNumVertices());
    for (unsigned int i = 0; i < points.size(); i+=3)
    {
        p[0] = pos[i * 3 + 0];
        p[1] = pos[i * 3 + 1];
        p[2] = pos[i * 3 + 2];
        point.New(p, &pnt);
        points[i] = pnt;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

bool CSubdivide::CreateSubdivPolygons(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons)
{
    printf("CSubdivide::CreateSubdivPolygons\n");
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);
    auto numFaces = refLastLevel.GetNumFaces();
    LXtPointID       verts[4];
    LXtPolygonID     polID;
    LXtID4           type = LXiPTYP_FACE;

    CLxUser_Polygon  upoly;
    m_edit_mesh.GetPolygons(upoly);

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

int CSubdivide::GetCagePolygon(int face, CLxUser_Polygon& upoly)
{
    printf("CSubdivide::GetCagePolygon\n");

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
    printf("CSubdivide::SetupCages\n");
	if (!m_mesh.test ())
		return false;

    m_polygons.clear();
    m_points.clear();
    m_polyFaceMap.clear();
    m_fvarArray.clear();

    CLxUser_Polygon upoly;
    m_mesh.GetPolygons(upoly);

	//
	// Collect polygons and their vertices from the source mesh.
	//
    printf("CSubdivide::SetupCages (1)\n");
    PolygonMeshBinVisitor vis(upoly, m_mbin, m_polygons);
    m_mbin.npols          = 0;
    m_mbin.polyVertCounts = 0;
    vis.m_upoly.Enum(&vis);
    if (m_mbin.npols == 0)
        return false;

	//
	// Create a map of polygon IDs to their indices.
	//
    printf("CSubdivide::SetupCages (2)\n");
    for (auto i = 0; i < m_polygons.size(); i++)
        m_polyFaceMap[m_polygons[i]] = i;

	//
	// Mark the vertices of the polygons to be subdivided.
	//
    CLxUser_MeshService mS;
    MarkingVisitor      mark;

    printf("CSubdivide::SetupCages (3)\n");
    mark.pol.fromMesh(m_mesh);
    mark.pnt.fromMesh(m_mesh);
    mark.clear = true;
    mark.mask  = mS.ClearMode(LXsMARK_USER_1);
    mark.pnt.Enum(&mark);
    mark.clear = false;
    mark.mask  = mS.SetMode(LXsMARK_USER_1);
    mark.pol.Enum(&mark);

	//
	// Collect the vertices of the marked polygons.
	//
    VertexTableVisitor vtab(m_points);

    printf("CSubdivide::SetupCages (4)\n");
    vtab.pnt.fromMesh(m_mesh);
    vtab.mask = mS.SetMode(LXsMARK_USER_1);
    vtab.pnt.Enum(&vtab);

    printf("CSubdivide::SetupCages Done\n");
    return true;
}

//
// Crate a new mesh interface with the specified bits and refiner.
//
Osd::GLMeshInterface* CSubdivide::CreateMeshInterface (Far::TopologyRefiner* refiner)
{
    printf("CSubdivide::CreateMeshInterface refiner (%p)\n", refiner);
    Osd::MeshBitset bits;
    bits.set(Osd::MeshAdaptive, m_adaptive);
    bits.set(Osd::MeshUseSmoothCornerPatch, false);
    bits.set(Osd::MeshUseSingleCreasePatch, false);
    bits.set(Osd::MeshUseInfSharpPatch, false);
    bits.set(Osd::MeshInterleaveVarying, true);
    bits.set(Osd::MeshFVarData, false);
    bits.set(Osd::MeshEndCapBilinearBasis, false);
    bits.set(Osd::MeshEndCapBSplineBasis, false);
    bits.set(Osd::MeshEndCapGregoryBasis, true);
    bits.set(Osd::MeshEndCapLegacyGregory, false);

	Osd::GLMeshInterface* glmesh = nullptr;
	int numVertexElements = 3;
	int numVaryingElements = 0;

	if (m_kernel == OSD_KERNEL_CPU) {
    printf("CSubdivide::OSD_KERNEL_CPU level (%u)\n", m_level);
		glmesh = new Osd::Mesh<Osd::CpuGLVertexBuffer,
			Far::StencilTable,
			Osd::CpuEvaluator,
			Osd::GLPatchTable> (
				refiner,
				numVertexElements,
				numVaryingElements,
				m_level, bits);
		printf ("CPU Kernel: glmesh %p\n", glmesh);
	}
#ifdef OPENSUBDIV_HAS_TBB
	else if (kernel == OSD_KERNEL_TBB) {
		glmesh = new Osd::Mesh<Osd::CpuGLVertexBuffer,
			Far::StencilTable,
			Osd::TbbEvaluator,
			Osd::GLPatchTable> (
				refiner,
				numVertexElements,
				numVaryingElements,
				m_level, bits);
		printf ("TBB Kernel: glmesh %p\n", glmesh);
	}
#endif
#ifdef OPENSUBDIV_HAS_OPENCL
	else if (kernel == OSD_KERNEL_CL) {
		// CLKernel
		static Osd::EvaluatorCacheT<Osd::CLEvaluator> clEvaluatorCache;
		glmesh = new Osd::Mesh<Osd::CLGLVertexBuffer,
			Osd::CLStencilTable,
			Osd::CLEvaluator,
			Osd::GLPatchTable,
			CLDeviceContext> (
				refiner,
				numVertexElements,
				numVaryingElements,
				m_level, bits,
				&clEvaluatorCache,
				&g_clDeviceContext);
		printf ("OpenCL Kernel: glmesh %p\n", glmesh);
	}
#endif
#ifdef OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK
	else if (kernel == OSD_KERNEL_GLSL) {
		static Osd::EvaluatorCacheT<Osd::GLXFBEvaluator> glXFBEvaluatorCache;
		glmesh = new Osd::Mesh<Osd::GLVertexBuffer,
			Osd::GLStencilTableTBO,
			Osd::GLXFBEvaluator,
			Osd::GLPatchTable>(
				refiner,
				numVertexElements,
				numVaryingElements,
				m_level, bits,
				&glXFBEvaluatorCache);
		printf ("GLSL Kernel: glmesh %p\n", glmesh);
	}
#endif
	else {
		glmesh = new Osd::Mesh<Osd::CpuGLVertexBuffer,
			Far::StencilTable,
			Osd::CpuEvaluator,
			Osd::GLPatchTable> (
				refiner,
				numVertexElements,
				numVaryingElements,
				m_level, bits);
		printf ("Default Kernel: glmesh %p\n", glmesh);
	}
	printf ("CreateContext: glmesh %p\n", glmesh);
	return glmesh;
}


Far::TopologyRefiner* CSubdivide::CreateTopologyRefiner()
{
    if (!m_mesh.test())
        return nullptr;

    printf("CSubdivide::CreateTopologyRefiner m_scheme %d\n", m_scheme);
    OpenSubdiv::Far::TopologyDescriptor desc{};

    CLxUser_Point upoint;
    m_mesh.GetPoints(upoint);
    desc.numVertices = static_cast<int>(m_points.size());
    if (!desc.numVertices)
        return nullptr;

    int npol = m_mesh.NPolygons();
    if (!npol)
        return nullptr;

    printf("CSubdivide::CreateTopologyRefiner npol = %d nvrt = %d\n", npol, desc.numVertices);
    //
    // Count number of subdiv polygons and total indices.
    //
    CLxUser_Polygon upoly;
    unsigned int    count;

    m_mesh.GetPolygons(upoly);
    desc.numFaces = 0;

	desc.numFaces = m_mbin.npols;
	auto faceVertCounts = m_mbin.polyVertCounts;
    printf("CSubdivide::CreateTopologyRefiner numFaces = %d faceVertCounts = %d\n", desc.numFaces, faceVertCounts);

    //
    // Store the number of face vertex and vertex indices.
    //
    std::vector<int> vertsPerFace;
    std::vector<int> vertIndices;

    vertsPerFace.resize(desc.numFaces);
    vertIndices.resize(faceVertCounts);

    LXtPointID point;

    std::map<LXtPointID, int> pointIndexMap;

    for (auto i = 0u; i < m_points.size(); i++)
        pointIndexMap[m_points[i]] = i;

	auto m = 0u;
    for (auto i = 0u; i < m_polygons.size(); i++)
    {
        upoly.Select(m_polygons[i]);
        upoly.VertexCount(&count);
        vertsPerFace[i] = static_cast<int>(count);
        for (auto j = 0u; j < count; j++)
        {
            upoly.VertexByIndex(j, &point);
            vertIndices[m++] = pointIndexMap[point];
        }
    }

    printf("CSubdivide::CreateTopologyRefiner (2)\n");
    desc.numVertsPerFace    = &vertsPerFace[0];
    desc.vertIndicesPerFace = &vertIndices[0];
    for (auto i = 0u; i < vertsPerFace.size(); i++)
        printf("vertsPerFace[%d] = %d\n", i, vertsPerFace[i]);
    for (auto i = 0u; i < vertIndices.size(); i++)
        printf("vertIndices[%d] = %d\n", i, vertIndices[i]);

    //
    // Edge creases/ vertex creases
    //
    std::vector<float> edgeCreases;
    std::vector<int>   edgeCreaseIndices;
    std::vector<float> vtxCreases;
    std::vector<int>   vtxCreaseIndices;

    CLxUser_Edge   edge;
    CLxLoc_MeshMap umap;
    LxResult       res;

    m_mesh.GetMaps(umap);
    res = umap.SelectByName(LXi_VMAP_SUBDIV, "Subdivision");
    if (res == LXe_OK)
    {
        m_mesh.GetEdges(edge);
        EdgeVisitor visEdge(umap, upoint, edge, edgeCreases, edgeCreaseIndices, pointIndexMap);
        visEdge.m_edge.Enum(&visEdge);
        VertexVisitor visVtx(umap, upoint, vtxCreases, vtxCreaseIndices, pointIndexMap);
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
    printf("CSubdivide::CreateTopologyRefiner (3)\n");
    MeshMapVisitor visMap(m_mesh, m_fvarArray, m_polygons);
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

    printf("CSubdivide::CreateTopologyRefiner (4)\n");
    Options options;
    options.SetVtxBoundaryInterpolation(m_boundary);
    options.SetFVarLinearInterpolation(m_fvar);

    Far::TopologyRefiner* refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc, Far::TopologyRefinerFactory<Descriptor>::Options(m_scheme, options));
    if (desc.numFVarChannels > 0)
        delete[] desc.fvarChannels;
    printf("CSubdivide::CreateTopologyRefiner (5)\n");
    return refiner;
}