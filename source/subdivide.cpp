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
#if 0
    m_glmesh = CreateMeshInterface(m_refiner);
    if (!m_glmesh)
        return false;
#endif

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
// Vertex container implementation.
//
struct Point3 {

    // Minimal required interface ----------------------
    Point3() { }

    void Clear( void * =0 ) {
        _point[0]=_point[1]=_point[2]=0.0f;
    }

    void AddWithWeight(Point3 const & src, float weight) {
        _point[0]+=weight*src._point[0];
        _point[1]+=weight*src._point[1];
        _point[2]+=weight*src._point[2];
    }

    // Public interface ------------------------------------
    void SetPoint(float x, float y, float z) {
        _point[0]=x;
        _point[1]=y;
        _point[2]=z;
    }

    const float * GetPoint() const {
        return _point;
    }

private:
    float _point[3];
};
struct FVarVertexUV {

    // Minimal required interface ----------------------
    void Clear() {
        u=v=0.0f;
    }

    void AddWithWeight(FVarVertexUV const & src, float weight) {
        u += weight * src.u;
        v += weight * src.v;
    }

    // Basic 'uv' layout channel
    float u,v;
};

//
// Refine subdivided positions from source mesh vertices.
//
bool CSubdivide::RefineGPU ()
{
    printf("CSubdivide::Refine m_adaptive = %d\n", m_adaptive);
	if (!m_mesh.test ())
		return false;

	CLxUser_Point   upoint;
	LXtFVector		pos;

	m_mesh.GetPoints (upoint);
	auto nvrt = m_points.size ();
	if (!nvrt)
		return false;

    std::vector<float> vertex(nvrt * 3);
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

    Far::TopologyLevel const& refCageLevel = m_refiner->GetLevel(0);
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);

    unsigned int size   = refLastLevel.GetNumVertices() * 3;
    unsigned int offset = refCageLevel.GetNumVertices() * 3;
    GLuint vbo = m_glmesh->BindVertexBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const float* posPtr = static_cast<const float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
    posPtr += offset;
    m_position.resize(refLastLevel.GetNumVertices() * 3);
    for (auto i = 0u; i < m_position.size(); i++)
    {
        m_position[i] = posPtr[i];
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

//
// Refine subdivided positions from source mesh vertices.
//
bool CSubdivide::Refine ()
{
    printf("CSubdivide::Refine m_adaptive = %d\n", m_adaptive);
	if (!m_mesh.test ())
		return false;

	CLxUser_Point   upoint;
	LXtFVector		pos;

	m_mesh.GetPoints (upoint);
	auto nvrt = m_points.size ();
	if (!nvrt)
		return false;

    int nRefinedLevels = static_cast<int>(m_level + 1);
    if (m_adaptive)
    {
        Far::PatchTableFactory::Options patchOptions(m_level);
        patchOptions.useInfSharpPatch = true;
        patchOptions.generateLegacySharpCornerPatches = false;
        patchOptions.generateVaryingTables = false;
        patchOptions.generateFVarTables = false;
        patchOptions.endCapType =
            Far::PatchTableFactory::Options::ENDCAP_GREGORY_BASIS;
        m_refiner->RefineAdaptive(patchOptions.GetRefineAdaptiveOptions());
    }
    else
        m_refiner->RefineUniform(Far::TopologyRefiner::UniformOptions(m_level));

    std::vector<Point3> vertex(m_refiner->GetNumVerticesTotal());
    for (auto i = 0u; i < nvrt; i++) {
        upoint.Select (m_points[i]);
        upoint.Pos (pos);
        vertex[i].SetPoint (pos[0], pos[1], pos[2]);
    }

    std::vector<std::vector<FVarVertexUV>> fvarArray(m_fvarArray.size());

    printf("m_fvarArray = %zu\n", m_fvarArray.size());
    for (auto i = 0u; i < m_fvarArray.size(); i++)
    {
        printf("[%u] indices = %zu values = %zu\n", i, m_fvarArray[i].indices.size(), m_fvarArray[i].values.size());
        fvarArray[i].resize(m_refiner->GetNumFVarValuesTotal(i));
        for (auto j = 0u; j < fvarArray[i].size(); j++)
        {
            fvarArray[i][j].u = m_fvarArray[i].values[j * 2];
            fvarArray[i][j].v = m_fvarArray[i].values[j * 2 + 1];
        }
    }

    // Interpolate vertex primvar data
    Far::PrimvarRefiner primvarRefiner(*m_refiner);

    if (m_adaptive)
        nRefinedLevels = static_cast<unsigned> (m_refiner->GetNumLevels());

printf("CSubdivide::Refine nRefinedLevels = %u / %u\n", nRefinedLevels, m_level);
    Point3 * src = &vertex[0];
    for (auto level = 1; level < nRefinedLevels; level++) {
        Point3 * dst = src + m_refiner->GetLevel(level-1).GetNumVertices();
        primvarRefiner.Interpolate(level, src, dst);
        src = dst;
        printf("-- level %d num vertices = %d\n", level, m_refiner->GetLevel(level).GetNumVertices());
    }

    std::vector<FVarVertexUV*> uvArray(fvarArray.size());
    for (auto i = 0u; i < fvarArray.size(); i++)
    {
        FVarVertexUV* srcUV = &fvarArray[i][0];
        for (auto level = 1; level < nRefinedLevels; level++) {
            FVarVertexUV * dstUV = srcUV + m_refiner->GetLevel(level-1).GetNumFVarValues(i);
            primvarRefiner.InterpolateFaceVarying(level, srcUV, dstUV, i);
            srcUV = dstUV;
        printf("-- level %d num fvar = %d\n", level, m_refiner->GetLevel(level).GetNumFVarValues(i));
        }
        uvArray[i] = srcUV;
    }

    Far::TopologyLevel const & refLastLevel = m_refiner->GetLevel(m_level);

    int nverts = refLastLevel.GetNumVertices();
    m_position.resize(nverts * 3);
    printf("CSubdivide::Refine nverts = %d total = %d\n", nverts, m_refiner->GetNumVerticesTotal());

    for (auto i = 0; i < nverts; i++) 
    {
        const float * pos = src[i].GetPoint();
        m_position[i * 3    ] = pos[0];
        m_position[i * 3 + 1] = pos[1];
        m_position[i * 3 + 2] = pos[2];
    //    printf("[%d] pos = %f %f %f\n", i, pos[0], pos[1], pos[2]);
    }

    m_uvs.resize(fvarArray.size());

    for (auto i = 0u; i < fvarArray.size(); i++)
    {
        int nuvs = refLastLevel.GetNumFVarValues(i);
        m_uvs[i].resize(nuvs * 2);
        FVarVertexUV* fvarUV = uvArray[i];
        for (auto j = 0; j < nuvs; j++) {
            m_uvs[i][j * 2    ] = fvarUV[j].u;
            m_uvs[i][j * 2 + 1] = fvarUV[j].v;
  //          printf("[%d] uv = %f %f\n", j, fvarUV[j].u, fvarUV[j].v);
        }
    }

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
    CreateSubdivUVs(points, polygons);
    RemoveSourcePolygons();
    printf("CSubdivide::Done\n");
	return true;
}

bool CSubdivide::CreateSubdivPoints(std::vector<LXtPointID>& points)
{
    printf("CSubdivide::CreateSubdivPoints = %zu\n", m_position.size());
    LXtVector       pos;
    LXtPointID      pnt;
    CLxUser_Point   point;
    point.fromMesh(m_edit_mesh);
    points.resize(m_position.size() / 3);
    for (auto i = 0u; i < points.size(); i++)
    {
        pos[0] = m_position[i * 3    ];
        pos[1] = m_position[i * 3 + 1];
        pos[2] = m_position[i * 3 + 2];
        point.New(pos, &pnt);
        points[i] = pnt;
    }
    return true;
}

bool CSubdivide::CreateSubdivPolygons(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons)
{
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);
    auto numFaces = refLastLevel.GetNumFaces();
    LXtPointID       verts[4];
    LXtPolygonID     polID;
    LXtID4           type = LXiPTYP_FACE;

    CLxUser_Polygon  upoly;
    m_edit_mesh.GetPolygons(upoly);
    printf("CSubdivide::CreateSubdivPolygons numFaces = %d points = %zu\n", numFaces, points.size());

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
        printf("[%d] fvers = %d (%d %d %d %d) verts %p %p %p %p\n", face, fverts.size(), 
            fverts[0], fverts[1], fverts[2], fverts[3], verts[0], verts[1], verts[2], verts[3]);
        polygons.push_back(polID);
    }
    return true;
}

bool CSubdivide::CreateSubdivUVs(std::vector<LXtPointID>& points, std::vector<LXtPolygonID>& polygons)
{
    printf("CSubdivide::CreateSubdivUVs\n");
    Far::TopologyLevel const& refLastLevel = m_refiner->GetLevel(m_level);
    auto numFaces = refLastLevel.GetNumFaces();
    LXtPointID       pnt;
    float            uv[2];

    CLxUser_Polygon  upoly;
    m_edit_mesh.GetPolygons(upoly);

	CLxUser_MeshMap	 umap;
	m_edit_mesh.GetMaps(umap);

    printf("m_fvarArray = %zu numFaces = %d points = %zu polygons = %zu\n", m_fvarArray.size(), numFaces, points.size(), polygons.size());
    for (auto iv = 0u; iv < m_fvarArray.size(); iv++)
    {
        umap.SelectByName (LXi_VMAP_TEXTUREUV, m_fvarArray[iv].name.c_str());
        LXtMeshMapID     mapID = umap.ID();
        printf("[%d] m_fvarArray indices = %zu values = %zu\n", iv, m_fvarArray[iv].indices.size(), m_fvarArray[iv].values.size());
        for (auto face = 0; face < numFaces; face++)
        {
            Far::ConstIndexArray fverts = refLastLevel.GetFaceVertices(face);
            Far::ConstIndexArray fuvs   = refLastLevel.GetFaceFVarValues(face, iv);
    printf("[%d] fverts = %d fuvs = %d polygon %p fverts %d %d %d %d\n", face, fverts.size(), fuvs.size(), polygons[face], fverts[0], fverts[1], fverts[2], fverts[3]);
            upoly.Select(polygons[face]);
            for (int i = 0; i < fverts.size(); i++) {
                auto j = static_cast<int>(fverts[i]);
                assert((j >= 0) && (j < points.size()));
                pnt = points[j];
                auto k = static_cast<int>(fuvs[i]);
                assert(pnt != nullptr);
                assert((k >= 0) && ((k * 2) < m_uvs[iv].size()));
                uv[0] = m_uvs[iv][k * 2];
                uv[1] = m_uvs[iv][k * 2 + 1];
                printf("[%d] pnt %p (%d) mapID %p uv %f %f\n", i, pnt, k, mapID, uv[0], uv[1]);
				upoly.SetMapValue (pnt, mapID, uv);
            }
        }
    }
    return true;
}

int CSubdivide::GetCagePolygon(int face, CLxUser_Polygon& upoly)
{
//    printf("CSubdivide::GetCagePolygon\n");

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

void CSubdivide::RemoveSourcePolygons()
{
    CLxUser_Polygon upoly;

    m_edit_mesh.GetPolygons(upoly);
    printf("CSubdivide::RemoveSourcePolygons = %zu\n", m_polygons.size());
   for (auto i = 0u; i < m_polygons.size(); i++)
   {
        upoly.Select(m_polygons[i]);
        upoly.Remove();
   }
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
    printf("CSubdivide::CreateTopologyRefiner numFaces = %d faceVertCounts = %d\n", desc.numFaces, faceVertCounts);

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
            for (auto j = 0; j < ntri; j++)
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