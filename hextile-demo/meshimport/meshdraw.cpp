#include "meshdraw.h"
#include "objreader.h"
#include "mikktspace.h"
#include "weldmesh.h"



struct STriOrQuad
{
	int orgFaceIdx;
	int orgVertIdx;
	int offsToTSpaces;
};


class CGenTSpace
{
public:
	CGenTSpace(CObjReader * importer, STriOrQuad * triOrQuadRefs, Vec4 * vTangents, const int nrTriOrQuad)
	{
		m_Importer = importer;
		m_TriOrQuadRefs = triOrQuadRefs;
		m_vTangents = vTangents;
		m_nrTriOrQuad = nrTriOrQuad;
	}

	virtual int GetNumFaces() const
	{
		return m_nrTriOrQuad;
	}

	virtual int GetNumVerticesOfFace(const int faceIdx) const
	{
		assert( faceIdx < GetNumFaces() );
		
		const int orgFaceIdx = m_TriOrQuadRefs[faceIdx].orgFaceIdx;
		const int nrVertsOnFace = m_Importer->GetNrFaceVertices(orgFaceIdx);
		return nrVertsOnFace==4 ? 4 : 3;		// ngons higher than 4 vertices are triangulated
	}
	// 0, 1, 2 for tri and 3 for quad
	virtual void GetPosition(float fvPosOut[], const int faceIdx, const int vertIdx) const
	{
		assert(faceIdx < GetNumFaces());
		assert(vertIdx>=0 && vertIdx<GetNumVerticesOfFace(faceIdx));

		const int orgFaceIdx = m_TriOrQuadRefs[faceIdx].orgFaceIdx;

		const int idx = UnpackIndex(orgFaceIdx, m_TriOrQuadRefs[faceIdx].orgVertIdx, vertIdx);

		const Vec3 P = m_Importer->GetFacePosition(orgFaceIdx, idx);
		fvPosOut[0] = P.x; fvPosOut[1] = P.y; fvPosOut[2] = P.z;
	}
	// 0, 1, 2 for tri and 3 for quad
	virtual void GetNormal(float fvNormOut[], const int faceIdx, const int vertIdx) const
	{
		assert(faceIdx < GetNumFaces());
		assert(vertIdx>=0 && vertIdx<GetNumVerticesOfFace(faceIdx));

		const int orgFaceIdx = m_TriOrQuadRefs[faceIdx].orgFaceIdx;

		const int idx = UnpackIndex(orgFaceIdx, m_TriOrQuadRefs[faceIdx].orgVertIdx, vertIdx);

		const Vec3 N = m_Importer->GetFaceNormal(orgFaceIdx, idx);
		fvNormOut[0] = N.x; fvNormOut[1] = N.y; fvNormOut[2] = N.z;

	}
	// 0, 1, 2 for tri and 3 for quad
	virtual void GetTexCoord(float fvTexcOut[], const int faceIdx, const int vertIdx) const
	{
		assert(faceIdx < GetNumFaces());
		assert(vertIdx>=0 && vertIdx<GetNumVerticesOfFace(faceIdx));

		const int orgFaceIdx = m_TriOrQuadRefs[faceIdx].orgFaceIdx;

		const int idx = UnpackIndex(orgFaceIdx, m_TriOrQuadRefs[faceIdx].orgVertIdx, vertIdx);

		const Vec3 T = m_Importer->GetFaceTexCoord(orgFaceIdx, idx);
		fvTexcOut[0] = T.x; fvTexcOut[1] = T.y;
	}
	// 0, 1, 2 for tri and 3 for quad
	virtual void GetTexCoord2(float fvTexcOut[], const int faceIdx, const int vertIdx) const
	{
		assert(faceIdx < GetNumFaces());
		assert(vertIdx>=0 && vertIdx<GetNumVerticesOfFace(faceIdx));

		const int orgFaceIdx = m_TriOrQuadRefs[faceIdx].orgFaceIdx;

		const int idx = UnpackIndex(orgFaceIdx, m_TriOrQuadRefs[faceIdx].orgVertIdx, vertIdx);

		const Vec3 T = m_Importer->GetFaceTexCoord2(orgFaceIdx, idx);
		fvTexcOut[0] = T.x; fvTexcOut[1] = T.y;
	}

	virtual void SetTSpaceBasic(const float fvTangent[], const float fSign, const int faceIdx, const int vertIdx)
	{
		assert(faceIdx < GetNumFaces());
		assert(vertIdx>=0 && vertIdx<GetNumVerticesOfFace(faceIdx));
		const int index = m_TriOrQuadRefs[faceIdx].offsToTSpaces + vertIdx;
		
		m_vTangents[index] = Vec4(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
	}

private:

	int UnpackIndex(const int orgFaceIdx, const int k, const int vertIdx) const
	{
		int idx = vertIdx;
		const int nrVertsOnFace = m_Importer->GetNrFaceVertices(orgFaceIdx);
		if(nrVertsOnFace==3 || nrVertsOnFace==4)
		{
			assert(nrVertsOnFace>=0 && nrVertsOnFace<=4);
		}
		else
		{
			assert(nrVertsOnFace>4 && k>=2);
			assert(vertIdx>=0 && vertIdx<4);
			idx = vertIdx==2 ? k : (vertIdx==1 ? (k-1) : 0);
		}

		return idx;
	}


	CObjReader * m_Importer;
	Vec4 * m_vTangents;			// (vOs, flip_factor)
	STriOrQuad * m_TriOrQuadRefs;

	int m_nrTriOrQuad;		// triangles and quads
};

// interface functions begin
static int getNumFaces(const SMikkTSpaceContext * pContext)
{
    return ((CGenTSpace *) pContext->m_pUserData)->GetNumFaces();
}

static int getNumVerticesOfFace(const SMikkTSpaceContext * pContext, const int faceIdx)
{
    return ((CGenTSpace *) pContext->m_pUserData)->GetNumVerticesOfFace(faceIdx);
}

static void getPosition(const SMikkTSpaceContext * pContext, float fvPosOut[], const int faceIdx, const int vertIdx)
{
    ((CGenTSpace *) pContext->m_pUserData)->GetPosition(fvPosOut, faceIdx, vertIdx);
}

static void getNormal(const SMikkTSpaceContext * pContext, float fvNormOut[], const int faceIdx, const int vertIdx)
{
    ((CGenTSpace *) pContext->m_pUserData)->GetNormal(fvNormOut, faceIdx, vertIdx);
}

static void getTexCoord(const SMikkTSpaceContext * pContext, float fvTexcOut[], const int faceIdx, const int vertIdx)
{
    ((CGenTSpace *) pContext->m_pUserData)->GetTexCoord(fvTexcOut, faceIdx, vertIdx);
}

static void setTSpaceBasic(const SMikkTSpaceContext * pContext, const float fvTangent[], const float sign, const int faceIdx, const int vertIdx)
{
    ((CGenTSpace *) pContext->m_pUserData)->SetTSpaceBasic(fvTangent, sign, faceIdx, vertIdx);
}
// interface functions end




bool CMeshDraw::ReadObj(ID3D11Device* pd3dDev, const char file_name[], const float fScale, const bool mustCenter, const bool bZtoYup)
{
	CObjReader importer;
	bool res = importer.ReadFile(file_name);

	const bool haveSecondaryUVs = importer.HaveSecondaryUVs();

	float * vertex_data_out = NULL;
	int * remapTable = NULL;
	int iNrUnique = 0;
	int nrTriangles = 0;

	const int nrSrcChannels = 3+3+4+2+(haveSecondaryUVs ? 2 : 0);


	if(res)
	{
		// determine number of triangles
		int maxNrTSpaces = 0;
		int nrTriOrQuad = 0;
		for(int f=0; f<importer.GetNumFaces(); f++)
		{
			const int nrFacesVerts = importer.GetNrFaceVertices(f);
			if(nrFacesVerts>2)
			{
				nrTriangles += (nrFacesVerts-2);

				nrTriOrQuad += nrFacesVerts==4 ? 1 : (nrFacesVerts-2);
				maxNrTSpaces += (nrFacesVerts<5 ? nrFacesVerts : (3*(nrFacesVerts-2)));
			}
		}

		// identify valid triangles and quads for tspace generation
		STriOrQuad * triOrQuadRefs = nrTriOrQuad>0 ? new STriOrQuad[nrTriOrQuad] : NULL;
		Vec4 * vTangents = maxNrTSpaces>0 ? new Vec4[maxNrTSpaces] : NULL;
		if(triOrQuadRefs!=NULL && vTangents!=NULL)
		{
			int idxTriOrQuad = 0;
			int idxTSpaceOffs = 0;
			for(int f=0; f<importer.GetNumFaces(); f++)
			{
				const int nrFacesVerts = importer.GetNrFaceVertices(f);
				if(nrFacesVerts==3 || nrFacesVerts==4)
				{
					STriOrQuad &triOrQuad = triOrQuadRefs[idxTriOrQuad];
					triOrQuad.orgFaceIdx = f; triOrQuad.orgVertIdx = 0;
					triOrQuad.offsToTSpaces = idxTSpaceOffs;

					++idxTriOrQuad;
					idxTSpaceOffs += nrFacesVerts;
				}
				else
				{
					// triangulate ngons with 5 verts or more
					assert(nrFacesVerts>2);
					for(int i=2; i<nrFacesVerts; i++)
					{
						STriOrQuad &triOrQuad = triOrQuadRefs[idxTriOrQuad];
						triOrQuad.orgFaceIdx = f; triOrQuad.orgVertIdx = i;
						triOrQuad.offsToTSpaces = idxTSpaceOffs;

						++idxTriOrQuad;
						idxTSpaceOffs += 3;
					}
				}
			}

			// setup mikktspace
			for(int i=0; i<maxNrTSpaces; i++)
			{ vTangents[i] = Vec4(1.0f, 0.0f, 0.0f, 1.0f); }

			CGenTSpace proxy_mesh(&importer, triOrQuadRefs, vTangents, nrTriOrQuad);

			SMikkTSpaceInterface sInterface;
			memset(&sInterface, 0, sizeof(sInterface));
			SMikkTSpaceContext sContext;
			sContext.m_pInterface = &sInterface;
			sContext.m_pUserData = (void *) &proxy_mesh;
			sInterface.m_getNumFaces = getNumFaces;
			sInterface.m_getNumVerticesOfFace = getNumVerticesOfFace;
			sInterface.m_getPosition = getPosition;
			sInterface.m_getNormal = getNormal;
			sInterface.m_getTexCoord = getTexCoord;
			sInterface.m_setTSpaceBasic = setTSpaceBasic;

			

			// triangulate all and weld
			if( genTangSpaceDefault(&sContext)!=0 )
			{
				float * vertex_data = new float[nrSrcChannels*nrTriangles*3];
				if(vertex_data!=NULL)
				{
					vertex_data_out = new float[nrSrcChannels*nrTriangles*3];
					remapTable = new int[nrTriangles*3];

					if(vertex_data_out!=NULL && remapTable!=NULL)
					{
						int triIdx = 0;
						for(int f=0; f<proxy_mesh.GetNumFaces(); f++)
						{
							const int nrVertsOnQuadOrTri = proxy_mesh.GetNumVerticesOfFace(f);
							assert(nrVertsOnQuadOrTri==3 || nrVertsOnQuadOrTri==4);

							bool splitAtFirst = true;
							if(nrVertsOnQuadOrTri==4) splitAtFirst = IsSplitQuadAtFirstVertex(importer, f);

							// 
							const int nrTrisOut = nrVertsOnQuadOrTri==4 ? 2 : 1;

							for(int k=0; k<nrTrisOut; k++)
							{
								const int indices[] = { (k==1 && (!splitAtFirst)) ? 1 : 0, (1+k), (k==0 && splitAtFirst) ? 2 : 3};
					
								for(int i=0; i<3; i++)
								{
									const int idx = indices[i];
									float * cur_vert_data = vertex_data + nrSrcChannels*(3*triIdx+i);
									proxy_mesh.GetPosition(cur_vert_data+0, f, idx);
									proxy_mesh.GetNormal(cur_vert_data+3, f, idx);
									memcpy(cur_vert_data+6, &vTangents[triOrQuadRefs[f].offsToTSpaces + idx], 4*sizeof(float));
									proxy_mesh.GetTexCoord(cur_vert_data+10, f, idx);
									if(haveSecondaryUVs)
										proxy_mesh.GetTexCoord2(cur_vert_data+12, f, idx);
								}
								++triIdx;
							}
						}

						// weld vertex buffer
						iNrUnique = WeldMesh(remapTable, vertex_data_out, (const float *) vertex_data, 3*nrTriangles, nrSrcChannels);
					}
				}

				if(vertex_data!=NULL) { delete [] vertex_data; vertex_data=NULL; }
			}
		}

		if(triOrQuadRefs!=NULL) { delete [] triOrQuadRefs; triOrQuadRefs=NULL; }
		if(vTangents!=NULL) { delete [] vTangents; vTangents=NULL; }
	}

	
	// copy data, scale and center
	res = iNrUnique>0 && remapTable!=NULL && vertex_data_out!=NULL;
	if(res)
	{
		m_vVerts = new SFilVert[iNrUnique];
		m_iIndices = new int[3*nrTriangles];

		if(m_vVerts!=NULL && m_iIndices!=NULL)
		{
			m_iNrVerts = iNrUnique;
			m_iNrTriangles = nrTriangles;

			// swap two indices
			for(int t=0; t<nrTriangles; t++)
			{
				m_iIndices[t*3+0] = remapTable[t*3+2];
				m_iIndices[t*3+1] = remapTable[t*3+1];
				m_iIndices[t*3+2] = remapTable[t*3+0];
			}

			// copy vertex data
			for(int i=0; i<iNrUnique; i++)
			{
				float * cur_vert_data = vertex_data_out + nrSrcChannels*i;
				m_vVerts[i].pos = Vec3(cur_vert_data[0], cur_vert_data[1], cur_vert_data[2]);
				m_vVerts[i].norm = Vec3(cur_vert_data[3], cur_vert_data[4], cur_vert_data[5]);
				m_vVerts[i].tang = Vec4(cur_vert_data[6], cur_vert_data[7], cur_vert_data[8], cur_vert_data[9]);
				m_vVerts[i].s = cur_vert_data[10]; m_vVerts[i].t = cur_vert_data[11];

				const float s2 = haveSecondaryUVs ? cur_vert_data[12] : 0.0f;
				const float t2 = haveSecondaryUVs ? cur_vert_data[13] : 0.0f;
				m_vVerts[i].s2 = s2; m_vVerts[i].t2 = t2;

				// 
				if(bZtoYup)
				{
					m_vVerts[i].pos = Vec3(m_vVerts[i].pos.x, m_vVerts[i].pos.z, -m_vVerts[i].pos.y);
					m_vVerts[i].norm = Vec3(m_vVerts[i].norm.x, m_vVerts[i].norm.z, -m_vVerts[i].norm.y);
					m_vVerts[i].tang = Vec4(m_vVerts[i].tang.x, m_vVerts[i].tang.z, -m_vVerts[i].tang.y, m_vVerts[i].tang.w);
				}
			}

			// scale and center
			Vec3 vMin=m_vVerts[0].pos * fScale;
			Vec3 vMax=vMin;
			for(int k=0; k<m_iNrVerts; k++)
			{
				m_vVerts[k].pos *= fScale;
				if(vMin.x>m_vVerts[k].pos.x) vMin.x=m_vVerts[k].pos.x;
				else if(vMax.x<m_vVerts[k].pos.x) vMax.x=m_vVerts[k].pos.x;
				if(vMin.y>m_vVerts[k].pos.y) vMin.y=m_vVerts[k].pos.y;
				else if(vMax.y<m_vVerts[k].pos.y) vMax.y=m_vVerts[k].pos.y;
				if(vMin.z>m_vVerts[k].pos.z) vMin.z=m_vVerts[k].pos.z;
				else if(vMax.z<m_vVerts[k].pos.z) vMax.z=m_vVerts[k].pos.z;
			}
			const Vec3 vCen = 0.5f*(vMax+vMin);

			if(mustCenter)
			{
				for(int k=0; k<m_iNrVerts; k++)
				{
					m_vVerts[k].pos -= vCen;
				}
				vMin -= vCen; vMax -= vCen;
			}
			m_vMin = vMin; m_vMax = vMax;
		}
		else res=false;
	}

	if(remapTable!=NULL) { delete [] remapTable; remapTable=NULL; }
	if(vertex_data_out!=NULL) { delete [] vertex_data_out; vertex_data_out=NULL; }

	// D3D11 specific buffer setup
	if(res)
	{
		HRESULT hr;

		// Set initial data info
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = m_vVerts;

		// Fill DX11 vertex buffer description
		D3D11_BUFFER_DESC     bd;
		bd.Usage =            D3D11_USAGE_DEFAULT;
		bd.ByteWidth =        sizeof( SFilVert ) * m_iNrVerts;
		bd.BindFlags =        D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags =   0;
		bd.MiscFlags =        0;

		// Create DX11 vertex buffer specifying initial data
		V( pd3dDev->CreateBuffer(&bd, &InitData, &m_pVertStream) );


		// Set initial data info
		InitData.pSysMem = m_iIndices;

		// Fill DX11 vertex buffer description
		bd.ByteWidth = sizeof(unsigned int) * m_iNrTriangles * 3;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		V( pd3dDev->CreateBuffer(&bd, &InitData, &m_pIndexStream) );
	}

	return res;
}

bool CMeshDraw::IsSplitQuadAtFirstVertex(CObjReader &importer, const int faceIdx) const
{
	bool res = true;

	const int iNrVerts = importer.GetNrFaceVertices(faceIdx);

	if(iNrVerts==4)
	{
		const Vec3 v0 = importer.GetFacePosition(faceIdx, 0);
		const Vec3 v1 = importer.GetFacePosition(faceIdx, 1);
		const Vec3 v2 = importer.GetFacePosition(faceIdx, 2);
		const Vec3 v3 = importer.GetFacePosition(faceIdx, 3);

		const float posLenDiag0 = LengthSquared(v2-v0);
		const float posLenDiag1 = LengthSquared(v3-v1);

		if(posLenDiag1<posLenDiag0)
			res = false;
		else if(posLenDiag0==posLenDiag1)
		{
			const Vec3 vDT0 = importer.GetFaceTexCoord(faceIdx, 2) - importer.GetFaceTexCoord(faceIdx, 0);
			const Vec3 vDT1 = importer.GetFaceTexCoord(faceIdx, 3) - importer.GetFaceTexCoord(faceIdx, 1);

			const float texLenDiag0 = vDT0.x*vDT0.x+vDT0.y*vDT0.y;
			const float texLenDiag1 = vDT1.x*vDT1.x+vDT1.y*vDT1.y;
			if(texLenDiag1<texLenDiag0)
				res = false;
		}
	}

	return res;
}



void CMeshDraw::CleanUp()
{
	if(m_pVertStream!=NULL) SAFE_RELEASE( m_pVertStream );
	if(m_pIndexStream!=NULL) SAFE_RELEASE( m_pIndexStream );

	if(m_iIndices!=NULL) { delete [] m_iIndices; m_iIndices=NULL; }
	if(m_vVerts!=NULL) { delete [] m_vVerts; m_vVerts=NULL; }
}


CMeshDraw::CMeshDraw()
{
	m_iNrVerts = 0;
	m_iNrTriangles = 0;
	m_vVerts = NULL;
	m_iIndices = NULL;

	m_pVertStream = NULL;
	m_pIndexStream = NULL;

	m_vMin = Vec3(0,0,0);
	m_vMax = Vec3(0,0,0);
}

CMeshDraw::~CMeshDraw()
{

}