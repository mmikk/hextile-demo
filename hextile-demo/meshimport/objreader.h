#ifndef __OBJREADER_H__
#define __OBJREADER_H__

#define DISABLE_QUAT

#include <geommath/geommath.h>

class CObjReader
{
public:
	bool ReadFile(const char name[]);

	//
	int GetNumFaces() const;
	int GetNrFaceVertices(const int iFaceIndex) const;

	const Vec3 GetFacePosition(const int iFaceIndex, const int iVertIndex) const;
	const Vec3 GetFaceTexCoord(const int iFaceIndex, const int iVertIndex) const;
	const Vec3 GetFaceTexCoord2(const int iFaceIndex, const int iVertIndex) const;
	const Vec3 GetFaceNormal(const int iFaceIndex, const int iVertIndex) const;
	const bool HaveSecondaryUVs() const;


	CObjReader();
	~CObjReader();

private:
	int RemoveWhiteSpace(const char * pText, const int iCurLocation, const int iSizeOfFile);
	int GoToChar(const char * pText, const int iCurLocation, const int iSizeOfFile, const char myChar);
	int SkipInteger(const char * pText, const int iCurLocation, const int iSizeOfFile);
	int ReadSingleFace(const char * pText, const int iCurLocation, const int iSizeOfFile);
	int MyAtof(float * pfRes, const char str[], const int iCurLocationIn, const int iSizeOfFile);

private:
	struct SFaceVertex
	{
		int iPosIndex;
		int iNrmIndex;
		int iTexIndex;
	};

	struct SFace
	{
		int iVertOffset;	// offset into face vertex buffer
		int iNrVertices;	// number of vertices on face.
	};

private:
	bool AddVertex(const Vec3 &vVert);
	bool AddTexCoord(const float s, const float t);
	bool AddTexCoord2(const float s, const float t);
	bool AddNormal(const Vec3 &vN);

	bool AddFaceVertex(const SFaceVertex &face_vertex);
	bool AddFace(const SFace &face);
	

	int m_iNrVertices, m_iNrVertsUpperBound;
	int m_iNrTexCoords, m_iNrTexCoordsUpperBound;
	int m_iNrTexCoords2, m_iNrTexCoordsUpperBound2;
	int m_iNrNormals, m_iNrNormalsUpperBound;

	Vec3 * m_pVerts;
	Vec3 * m_pTexCoords;
	Vec3 * m_pTexCoords2;
	Vec3 * m_pNormals;
	
	int m_iNrFaces, m_iNrFacesUpperBound;
	int m_iNrFaceVertices, m_iNrFaceVerticesUpperBound;

	SFace * m_pFaces;
	SFaceVertex * m_pFaceVertices;

	void CleanUp();
};

#endif