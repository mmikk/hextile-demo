#ifndef __MESHDRAW_H__
#define __MESHDRAW_H__

#include "DXUT.h"

#define DISABLE_QUAT

#include <geommath/geommath.h>


class CObjReader;

struct SFilVert
{
	Vec3 pos;	// position
	Vec3 norm;				// normal
	Vec4 tang;

	float s, t;	// texcoord
	float s2, t2;	// texcoord 2
};



class CMeshDraw
{
public:
	bool ReadObj(ID3D11Device* pd3dDev, const char file_name[], const float fScale = 1.0f, const bool bCenter = true, const bool bZtoYup=false);
	void CleanUp();
	int GetNrTriangles() { return m_iNrTriangles; }
	ID3D11Buffer *const * GetVertexBuffer() { return &m_pVertStream; }
	ID3D11Buffer * GetIndexBuffer() { return m_pIndexStream; }
	const Vec3 GetMin() const { return m_vMin; }
	const Vec3 GetMax() const { return m_vMax; }


	CMeshDraw();
	~CMeshDraw();


private:
	bool IsSplitQuadAtFirstVertex(CObjReader &importer, const int faceIdx) const;

	int m_iNrVerts;
	int m_iNrTriangles;
	SFilVert * m_vVerts;
	int * m_iIndices;

	Vec3 m_vMin, m_vMax;

private:	// d3d specific
	ID3D11Buffer * m_pVertStream;
	ID3D11Buffer * m_pIndexStream;
};

#endif