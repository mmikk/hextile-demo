#include "objreader.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <float.h>

#include "weldmesh.h"

int CObjReader::GetNumFaces() const
{
	return m_iNrFaces;
}

int CObjReader::GetNrFaceVertices(const int iFaceIndex) const
{
	assert(iFaceIndex>=0 && iFaceIndex<GetNumFaces());
	return m_pFaces[iFaceIndex].iNrVertices;
}

const Vec3 CObjReader::GetFacePosition(const int iFaceIndex, const int iVertIndex) const
{
	assert(iFaceIndex>=0 && iFaceIndex<m_iNrFaces);
	const SFace &face = m_pFaces[iFaceIndex];
	assert(iVertIndex>=0 && iVertIndex<face.iNrVertices);
	const SFaceVertex &face_vertex = m_pFaceVertices[face.iVertOffset + iVertIndex];
	assert(face_vertex.iPosIndex>=0 && face_vertex.iPosIndex<m_iNrVertices);
	return m_pVerts[ face_vertex.iPosIndex ];
}

const Vec3 CObjReader::GetFaceTexCoord(const int iFaceIndex, const int iVertIndex) const
{
	assert(iFaceIndex>=0 && iFaceIndex<m_iNrFaces);
	const SFace &face = m_pFaces[iFaceIndex];
	assert(iVertIndex>=0 && iVertIndex<face.iNrVertices);
	const SFaceVertex &face_vertex = m_pFaceVertices[face.iVertOffset + iVertIndex];
	if( face_vertex.iTexIndex>=0 && face_vertex.iTexIndex<m_iNrTexCoords )
		return m_pTexCoords[ face_vertex.iTexIndex ];
	else 
		return Vec3(0,0,1);
}

const Vec3 CObjReader::GetFaceTexCoord2(const int iFaceIndex, const int iVertIndex) const
{
	assert(iFaceIndex>=0 && iFaceIndex<m_iNrFaces);
	const SFace &face = m_pFaces[iFaceIndex];
	assert(iVertIndex>=0 && iVertIndex<face.iNrVertices);
	const SFaceVertex &face_vertex = m_pFaceVertices[face.iVertOffset + iVertIndex];

	// assume secondary texture coordinates use the same index as the primary set
	if( face_vertex.iTexIndex>=0 && face_vertex.iTexIndex<m_iNrTexCoords2 )
		return m_pTexCoords2[ face_vertex.iTexIndex ];
	else 
		return Vec3(0,0,1);
}

const Vec3 CObjReader::GetFaceNormal(const int iFaceIndex, const int iVertIndex) const
{
	assert(iFaceIndex>=0 && iFaceIndex<m_iNrFaces);
	const SFace &face = m_pFaces[iFaceIndex];
	assert(iVertIndex>=0 && iVertIndex<face.iNrVertices);
	const SFaceVertex &face_vertex = m_pFaceVertices[face.iVertOffset + iVertIndex];
	assert(face_vertex.iNrmIndex>=0 && face_vertex.iNrmIndex<m_iNrNormals);
	return m_pNormals[ face_vertex.iNrmIndex ];
}

const bool CObjReader::HaveSecondaryUVs() const
{
	return m_iNrTexCoords2>0;
}

static float Clamp(const float x, const float a, const float b) { return x>b?b:(x<a?a:x); }

bool CObjReader::ReadFile(const char name[])
{
	CleanUp();

	FILE * fptr = fopen(name, "rb");
	if(fptr!=NULL)
	{
		fseek(fptr, 0, SEEK_SET);
		const int iStart = ftell(fptr);
		fseek(fptr, 0, SEEK_END);
		const int iEnd = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);

		int iSizeOfFile = iEnd-iStart;

		char * pText = new char[iSizeOfFile];
		fread(pText, 1, iSizeOfFile, fptr);
		fclose(fptr); fptr=NULL;

		int iCurLocation = 0;
		int iPrePerc = -1;

		while(iCurLocation<iSizeOfFile)
		{
			// first remove white space including coder comments
			iCurLocation = RemoveWhiteSpace(pText, iCurLocation, iSizeOfFile);

			if(iCurLocation>=iSizeOfFile) break;

			if( strncmp(pText+iCurLocation, "v ", 2)==0 || strncmp(pText+iCurLocation, "v\t", 2)==0 )
			{
				// if this is a vertex
				float vx, vy, vz;
				//const int iItems = sscanf(pText+iCurLocation, "v%f%f%f\n", &vx, &vy, &vz);
				//assert(iItems==3);
				int curLocation2 = MyAtof(&vx, pText, iCurLocation+1, iSizeOfFile);
				curLocation2 = MyAtof(&vy, pText, curLocation2, iSizeOfFile);
				curLocation2 = MyAtof(&vz, pText, curLocation2, iSizeOfFile);
				
				AddVertex(Vec3(vx, vy, vz));
			}
			else if( strncmp(pText+iCurLocation, "vt ", 3)==0 || strncmp(pText+iCurLocation, "vt\t", 3)==0 )
			{
				// if this is a texture coordinate
				float s, t;
				//const int iItems = sscanf(pText+iCurLocation, "vt%f%f\n", &s, &t);
				//assert(iItems==2);
				int curLocation2 = MyAtof(&s, pText, iCurLocation+2, iSizeOfFile);
				curLocation2 = MyAtof(&t, pText, curLocation2, iSizeOfFile);
				AddTexCoord(s, t);
			}
			else if( strncmp(pText+iCurLocation, "#vt2 ", 5)==0 || strncmp(pText+iCurLocation, "#vt2\t", 5)==0 )
			{
				// if this is a texture coordinate
				float s, t;
				//const int iItems = sscanf(pText+iCurLocation, "#vt2%f%f\n", &s, &t);
				//assert(iItems==2);
				int curLocation2 = MyAtof(&s, pText, iCurLocation+4, iSizeOfFile);
				curLocation2 = MyAtof(&t, pText, curLocation2, iSizeOfFile);
				AddTexCoord2(s, t);
			}
			else if( strncmp(pText+iCurLocation, "vn ", 3)==0 || strncmp(pText+iCurLocation, "vn\t", 3)==0)
			{
				// if this is a normal
				float nx, ny, nz;
				//const int iItems = sscanf(pText+iCurLocation, "vn%f%f%f\n", &nx, &ny, &nz);
				//assert(iItems==3);
				int curLocation2 = MyAtof(&nx, pText, iCurLocation+2, iSizeOfFile);
				curLocation2 = MyAtof(&ny, pText, curLocation2, iSizeOfFile);
				curLocation2 = MyAtof(&nz, pText, curLocation2, iSizeOfFile);
				Vec3 vN = Vec3(nx, ny, nz);
				if( LengthSquared(vN)>=FLT_EPSILON )
					vN = Normalize(vN);
				else
					vN = Vec3(1.0f, 0.0f, 0.0f);
				AddNormal(vN);
			}
			else if(pText[iCurLocation]=='g' || pText[iCurLocation]=='s')
			{
			}
			else if(pText[iCurLocation]=='f')
			{
				iCurLocation = ReadSingleFace(pText, iCurLocation, iSizeOfFile);
			}

			// next line
			iCurLocation = GoToChar(pText, iCurLocation, iSizeOfFile, '\n');

			int iCurPerc = (iCurLocation*100+(iSizeOfFile/2)) / iSizeOfFile;
			if(iCurPerc!=iPrePerc)
			{
				printf("work complete: %d\n", iCurPerc);
				iPrePerc = iCurPerc;
			}
		}

		delete [] pText;
	}
	else
		return false;

	// create vertex offsets for faces/polygons
	int iOffset = 0;
	for(int f=0; f<GetNumFaces(); f++)
	{
		m_pFaces[f].iVertOffset = iOffset;
		const int iVertsOnFace = m_pFaces[f].iNrVertices;
		assert(iVertsOnFace>0);
		iOffset += iVertsOnFace;
	}
	assert(m_iNrFaceVertices==iOffset);

	// create normals if none were found
	if(m_iNrNormals<=0 && m_pNormals==NULL)
	{
		Vec3 * pfVertexDataOut = new Vec3[m_iNrVertices];
		int * piRemapTable = new int[m_iNrVertices];

		if(m_pVerts!=NULL && pfVertexDataOut!=NULL && piRemapTable!=NULL)
		{
			printf("Welding vertex data.\n");
			// weld vertex buffer
			const int iNrUnique =
				WeldMesh(piRemapTable, (float *) pfVertexDataOut, (const float *) m_pVerts, m_iNrVertices, 3);

			// create vertex normals index list
			m_iNrNormals = iNrUnique;
			for(int i=0; i<m_iNrFaceVertices; i++)
				m_pFaceVertices[i].iNrmIndex = piRemapTable[m_pFaceVertices[i].iPosIndex];

			// initialize vnormals (deleting welded vertices)
			m_pNormals = pfVertexDataOut;
			for(int n=0; n<iNrUnique; n++) m_pNormals[n]=Vec3(0,0,0);

			// 
			for(int f=0; f<GetNumFaces(); f++)
			{
				const int offset = m_pFaces[f].iVertOffset;
				const int nrverts = m_pFaces[f].iNrVertices;
				for(int i=2; i<nrverts; i++)
				{
					const int index0 = 0;
					const int index1 = i-1;
					const int index2 = i;

					const SFaceVertex &vert0 = m_pFaceVertices[offset+index0];
					const SFaceVertex &vert1 = m_pFaceVertices[offset+index1];
					const SFaceVertex &vert2 = m_pFaceVertices[offset+index2];

					const Vec3 &p0 = m_pVerts[ vert0.iPosIndex ];
					const Vec3 &p1 = m_pVerts[ vert1.iPosIndex ];
					const Vec3 &p2 = m_pVerts[ vert2.iPosIndex ];

					const int n0 = vert0.iNrmIndex, n1 = vert1.iNrmIndex, n2 = vert2.iNrmIndex;

					if(n0!=n1 && n1!=n2 && n0!=n2)
					{
						const Vec3 v0 = Normalize(p1-p0);
						const Vec3 v1 = Normalize(p2-p1);
						const Vec3 v2 = Normalize(p0-p2);
						const float dot0 = Clamp(-(v0.x*v2.x+v0.y*v2.y+v0.z*v2.z),-1,1);
						const float dot1 = Clamp(-(v1.x*v0.x+v1.y*v0.y+v1.z*v0.z),-1,1);
						const float dot2 = Clamp(-(v2.x*v1.x+v2.y*v1.y+v2.z*v1.z),-1,1);
						const double dA0 = acos(dot0);
						const double dA1 = acos(dot1);
						const double dA2 = acos(dot2);

						const Vec3 vNeg(-v2.x, -v2.y, -v2.z);
						const Vec3 vN = Normalize(Cross(v0,vNeg));
						m_pNormals[n0] += (dA0*vN); m_pNormals[n1] += (dA1*vN); m_pNormals[n2] += (dA2*vN);
					}
				}
			}

			// normalize
			for(int n=0; n<m_iNrNormals; n++)
            {
                if(m_pNormals[n]!=Vec3(0,0,0))
                    m_pNormals[n]=Normalize(m_pNormals[n]);
                else
                    m_pNormals[n]=Vec3(0,0,1);
            }
		}

		if(piRemapTable!=NULL) delete [] piRemapTable;
	}

	// return with success
	return true;
}


int CObjReader::MyAtof(float * pfRes, const char str[], const int iCurLocationIn, const int iSizeOfFile)
{
	int iCurLocation = RemoveWhiteSpace(str, iCurLocationIn, iSizeOfFile);
	pfRes[0] = (float) atof(str+iCurLocation);

	while(iCurLocation<iSizeOfFile && (isdigit(str[iCurLocation]) || str[iCurLocation]=='.' || tolower(str[iCurLocation])=='f' || tolower(str[iCurLocation])=='e' || str[iCurLocation]=='-' || str[iCurLocation]=='+'))
		++iCurLocation;

	return iCurLocation;
}


int CObjReader::RemoveWhiteSpace(const char * pText, const int iCurLocation, const int iSizeOfFile)
{
	bool bNotFoundHackEntry = true;
	int iCur = iCurLocation;
	while((iCur<iSizeOfFile) && bNotFoundHackEntry && (pText[iCur]==' ' || pText[iCur]=='\t' ||
		  /*pText[iCur]=='\n' ||*/ pText[iCur]=='#'))
	{
		if(pText[iCur]=='#')
		{
			if( (iCur+4)<iSizeOfFile && pText[iCur+1]=='v' && pText[iCur+2]=='t' && pText[iCur+3]=='2' &&
				(pText[iCur+4]==' ' || pText[iCur+4]=='\t') ) 
			{
				bNotFoundHackEntry=false;
			}

			if(bNotFoundHackEntry)
			{
				while((iCur<iSizeOfFile) && pText[iCur]!='\n') ++iCur;
			}
		}
		if(iCur<iSizeOfFile && bNotFoundHackEntry) ++iCur;
	}
	return iCur;
}

int CObjReader::GoToChar(const char * pText, const int iCurLocation, const int iSizeOfFile, const char myChar)
{
	int iCur = iCurLocation;
	while((iCur<iSizeOfFile) && pText[iCur]!=myChar) ++iCur;
		++iCur;
	return iCur;
}

int CObjReader::SkipInteger(const char * pText, const int iCurLocation, const int iSizeOfFile)
{
	int iCur = iCurLocation;
	while((iCur<iSizeOfFile) && isdigit(pText[iCur])!=0) ++iCur;
	iCur = RemoveWhiteSpace(pText, iCur, iSizeOfFile);
	return iCur;
}

int CObjReader::ReadSingleFace(const char * pText, const int iCurLocation, const int iSizeOfFile)
{
	int iCur = iCurLocation;

	const bool bIsFace = strncmp(pText+iCur, "f ", 2)==0 || strncmp(pText+iCur, "f\t", 2)==0;
	assert(bIsFace);

	// skip f
	++iCur;

	iCur = RemoveWhiteSpace(pText, iCur, iSizeOfFile);
	int i=0;
	while(iCur<iSizeOfFile && isdigit(pText[iCur])!=0)
	{
		// read vertex index
		SFaceVertex data;
		data.iPosIndex = -1; data.iTexIndex = -1; data.iNrmIndex = -1;
		data.iPosIndex = atoi(pText+iCur) - 1;
		assert(data.iPosIndex>=0);
		iCur = SkipInteger(pText, iCur, iSizeOfFile);

		// read texture index and normal
		if(pText[iCur]=='/')
		{
			// texture coordinate comes first
			++iCur;
			if(isdigit(pText[iCur]))	// texture coordinate
			{
				data.iTexIndex = atoi(pText+iCur) - 1;
				iCur = SkipInteger(pText, iCur, iSizeOfFile);
			}
			else
			{
				// no texture coordinate but since we
				// found a slash there has to be a normal
				assert(pText[iCur]=='/');
			}

			// look for normal
			if(pText[iCur]=='/')
			{
				++iCur;
				if(isdigit(pText[iCur]))
				{
					data.iNrmIndex = atoi(pText+iCur) - 1;
					iCur = SkipInteger(pText, iCur, iSizeOfFile);
				}
			}
		}

		// finish up
		iCur = RemoveWhiteSpace(pText, iCur, iSizeOfFile);

		// next triangle
		AddFaceVertex(data);

		// next vertex
		++i;
	}

	SFace face;
	face.iNrVertices = i; face.iVertOffset = 0;	// offsets are determined later
	AddFace(face);

	return iCur;
}

bool CObjReader::AddVertex(const Vec3 &vVert)
{
	if(m_iNrVertices>=m_iNrVertsUpperBound)
	{
		assert(m_iNrVertices==m_iNrVertsUpperBound);

		const int iAlloc = m_iNrVertsUpperBound==0?1:(2*m_iNrVertsUpperBound);
		Vec3 * pvNew = new Vec3[iAlloc];
		if(pvNew!=NULL)
		{
			for(int q=0; q<m_iNrVertices; q++) pvNew[q] = m_pVerts[q];
			if(m_pVerts!=NULL) delete [] m_pVerts;
			m_pVerts = pvNew;
			m_iNrVertsUpperBound = iAlloc;
		}
		else return false;
	}

	assert(m_iNrVertices<m_iNrVertsUpperBound);
	m_pVerts[m_iNrVertices++] = vVert;

	return true;
}

bool CObjReader::AddTexCoord(const float s, const float t)
{
	if(m_iNrTexCoords>=m_iNrTexCoordsUpperBound)
	{
		assert(m_iNrTexCoords==m_iNrTexCoordsUpperBound);

		const int iAlloc = m_iNrTexCoordsUpperBound==0?1:(2*m_iNrTexCoordsUpperBound);
		Vec3 * pvNew = new Vec3[iAlloc];
		if(pvNew!=NULL)
		{
			for(int q=0; q<m_iNrTexCoords; q++) pvNew[q] = m_pTexCoords[q];
			if(m_pTexCoords!=NULL) delete [] m_pTexCoords;
			m_pTexCoords = pvNew;
			m_iNrTexCoordsUpperBound = iAlloc;
		}
		else return false;
	}

	assert(m_iNrTexCoords<m_iNrTexCoordsUpperBound);
	m_pTexCoords[m_iNrTexCoords+0] = Vec3(s, t, 1);
	++m_iNrTexCoords;

	return true;
}

bool CObjReader::AddTexCoord2(const float s, const float t)
{
	if(m_iNrTexCoords2>=m_iNrTexCoordsUpperBound2)
	{
		assert(m_iNrTexCoords2==m_iNrTexCoordsUpperBound2);

		const int iAlloc = m_iNrTexCoordsUpperBound2==0?1:(2*m_iNrTexCoordsUpperBound2);
		Vec3 * pvNew = new Vec3[iAlloc];
		if(pvNew!=NULL)
		{
			for(int q=0; q<m_iNrTexCoords2; q++) pvNew[q] = m_pTexCoords2[q];
			if(m_pTexCoords2!=NULL) delete [] m_pTexCoords2;
			m_pTexCoords2 = pvNew;
			m_iNrTexCoordsUpperBound2 = iAlloc;
		}
		else return false;
	}

	assert(m_iNrTexCoords2<m_iNrTexCoordsUpperBound2);
	m_pTexCoords2[m_iNrTexCoords2+0] = Vec3(s, t, 1);
	++m_iNrTexCoords2;

	return true;
}

bool CObjReader::AddNormal(const Vec3 &vN)
{
	if(m_iNrNormals>=m_iNrNormalsUpperBound)
	{
		assert(m_iNrNormals==m_iNrNormalsUpperBound);

		const int iAlloc = m_iNrNormalsUpperBound==0?1:(2*m_iNrNormalsUpperBound);
		Vec3 * pvNew = new Vec3[iAlloc];
		if(pvNew!=NULL)
		{
			for(int q=0; q<m_iNrNormals; q++) pvNew[q] = m_pNormals[q];
			if(m_pNormals!=NULL) delete [] m_pNormals;
			m_pNormals = pvNew;
			m_iNrNormalsUpperBound = iAlloc;
		}
		else return false;
	}

	assert(m_iNrNormals<m_iNrNormalsUpperBound);
	m_pNormals[m_iNrNormals++] = vN;

	return true;
}

bool CObjReader::AddFaceVertex(const SFaceVertex &face_vertex)
{
	if(m_iNrFaceVertices>=m_iNrFaceVerticesUpperBound)
	{
		assert(m_iNrFaceVertices==m_iNrFaceVerticesUpperBound);

		const int iAlloc = m_iNrFaceVerticesUpperBound==0?1:(2*m_iNrFaceVerticesUpperBound);
		SFaceVertex * pfNew = new SFaceVertex[iAlloc];
		if(pfNew!=NULL)
		{
			for(int q=0; q<m_iNrFaceVertices; q++) pfNew[q] = m_pFaceVertices[q];
			if(m_pFaceVertices!=NULL) delete [] m_pFaceVertices;
			m_pFaceVertices = pfNew;
			m_iNrFaceVerticesUpperBound = iAlloc;
		}
		else return false;
	}

	assert(m_iNrFaceVertices<m_iNrFaceVerticesUpperBound);
	m_pFaceVertices[m_iNrFaceVertices++] = face_vertex;

	return true;
}

bool CObjReader::AddFace(const SFace &face)
{
	if(m_iNrFaces>=m_iNrFacesUpperBound)
	{
		assert(m_iNrFaces==m_iNrFacesUpperBound);

		const int iAlloc = m_iNrFacesUpperBound==0?1:(2*m_iNrFacesUpperBound);
		SFace * pfNew = new SFace[iAlloc];
		if(pfNew!=NULL)
		{
			for(int q=0; q<m_iNrFaces; q++) pfNew[q] = m_pFaces[q];
			if(m_pFaces!=NULL) delete [] m_pFaces;
			m_pFaces = pfNew;
			m_iNrFacesUpperBound = iAlloc;
		}
		else return false;
	}

	assert(m_iNrFaces<m_iNrFacesUpperBound);
	m_pFaces[m_iNrFaces++] = face;

	return true;
}


void CObjReader::CleanUp()
{
	m_iNrVertices=0; m_iNrVertsUpperBound=0;
	m_iNrTexCoords=0; m_iNrTexCoordsUpperBound=0;
	m_iNrTexCoords2=0; m_iNrTexCoordsUpperBound2=0;
	m_iNrNormals=0; m_iNrNormalsUpperBound=0;

	if(m_pVerts != NULL) { delete [] m_pVerts; m_pVerts=NULL; }
	if(m_pTexCoords != NULL) { delete [] m_pTexCoords; m_pTexCoords=NULL; }
	if(m_pTexCoords2 != NULL) { delete [] m_pTexCoords2; m_pTexCoords2=NULL; }
	if(m_pNormals != NULL) { delete [] m_pNormals; m_pNormals=NULL; }

	m_iNrFaces=0; m_iNrFacesUpperBound=0;
	m_iNrFaceVertices=0; m_iNrFaceVerticesUpperBound=0;

	if(m_pFaces != NULL) { delete [] m_pFaces; m_pFaces=NULL; }
	if(m_pFaceVertices != NULL) { delete [] m_pFaceVertices; m_pFaceVertices=NULL; }
}
CObjReader::CObjReader()
{
	m_iNrVertices=0; m_iNrVertsUpperBound=0;
	m_iNrTexCoords=0; m_iNrTexCoordsUpperBound=0;
	m_iNrTexCoords2=0; m_iNrTexCoordsUpperBound2=0;
	m_iNrNormals=0; m_iNrNormalsUpperBound=0;
	
	m_pVerts = NULL; m_pTexCoords = NULL;
	m_pNormals = NULL; m_pTexCoords2 = NULL;

	m_iNrFaces=0; m_iNrFacesUpperBound=0;
	m_iNrFaceVertices=0; m_iNrFaceVerticesUpperBound=0;

	m_pFaces = NULL; m_pFaceVertices = NULL;
}

CObjReader::~CObjReader()
{
	CleanUp();
}