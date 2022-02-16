#include "mat33.h"
#include "vec3.h"
#include "quaternion.h"
#include <math.h>




const Vec3	operator *( const Mat33 &m, const Vec3 &v )
{
	Vec3 r;

	r.x = m.m_fMat[0+0*3]*v.x + m.m_fMat[0+1*3]*v.y + m.m_fMat[0+2*3]*v.z;
	r.y = m.m_fMat[1+0*3]*v.x + m.m_fMat[1+1*3]*v.y + m.m_fMat[1+2*3]*v.z;
	r.z = m.m_fMat[2+0*3]*v.x + m.m_fMat[2+1*3]*v.y + m.m_fMat[2+2*3]*v.z;

	return r;
}

const Mat33	operator *( const Mat33 &m1, const Mat33 &m2 )
{
	Mat33 r;

	for(int iCol = 0; iCol < 3; iCol++)
	{
		r.m_fMat[0+iCol*3] = m1.m_fMat[0+0*3]*m2.m_fMat[0+iCol*3] + m1.m_fMat[0+1*3]*m2.m_fMat[1+iCol*3] + m1.m_fMat[0+2*3]*m2.m_fMat[2+iCol*3];
		r.m_fMat[1+iCol*3] = m1.m_fMat[1+0*3]*m2.m_fMat[0+iCol*3] + m1.m_fMat[1+1*3]*m2.m_fMat[1+iCol*3] + m1.m_fMat[1+2*3]*m2.m_fMat[2+iCol*3];
		r.m_fMat[2+iCol*3] = m1.m_fMat[2+0*3]*m2.m_fMat[0+iCol*3] + m1.m_fMat[2+1*3]*m2.m_fMat[1+iCol*3] + m1.m_fMat[2+2*3]*m2.m_fMat[2+iCol*3];
	}

	return r;
}

Mat33&		operator *=( Mat33 &m1, const Mat33 &m2 )
{
	m1 = m2 * m1;		// We add m2 to the heap of transformations

	return m1;
}

const Mat33	operator ~( const Mat33 &m )
{
	Mat33 r;

	float s = 1.0f / Determinant( m );

	r.m_fMat[0+0*3] = s * (m.m_fMat[1+1*3]*m.m_fMat[2+2*3] - m.m_fMat[1+2*3]*m.m_fMat[2+1*3]);
	r.m_fMat[0+1*3] = -s * (m.m_fMat[0+1*3]*m.m_fMat[2+2*3] - m.m_fMat[0+2*3]*m.m_fMat[2+1*3]);
	r.m_fMat[0+2*3] = s * (m.m_fMat[0+1*3]*m.m_fMat[1+2*3] - m.m_fMat[0+2*3]*m.m_fMat[1+1*3]);

	r.m_fMat[1+0*3] = -s * (m.m_fMat[1+0*3]*m.m_fMat[2+2*3] - m.m_fMat[1+2*3]*m.m_fMat[2+0*3]);
	r.m_fMat[1+1*3] = s * (m.m_fMat[0+0*3]*m.m_fMat[2+2*3] - m.m_fMat[0+2*3]*m.m_fMat[2+0*3]);
	r.m_fMat[1+2*3] = -s * (m.m_fMat[0+0*3]*m.m_fMat[1+2*3] - m.m_fMat[0+2*3]*m.m_fMat[1+0*3]);

	r.m_fMat[2+0*3] = s * (m.m_fMat[1+0*3]*m.m_fMat[2+1*3] - m.m_fMat[1+1*3]*m.m_fMat[2+0*3]);
	r.m_fMat[2+1*3] = -s * (m.m_fMat[0+0*3]*m.m_fMat[2+1*3] - m.m_fMat[0+1*3]*m.m_fMat[2+0*3]);
	r.m_fMat[2+2*3] = s * (m.m_fMat[0+0*3]*m.m_fMat[1+1*3] - m.m_fMat[0+1*3]*m.m_fMat[1+0*3]);

	return r;
}

float		Determinant( const Mat33 &m )
{
	return m.m_fMat[0+0*3]*(m.m_fMat[1+1*3]*m.m_fMat[2+2*3] - m.m_fMat[1+2*3]*m.m_fMat[2+1*3]) -
		   m.m_fMat[0+1*3]*(m.m_fMat[1+0*3]*m.m_fMat[2+2*3] - m.m_fMat[1+2*3]*m.m_fMat[2+0*3]) +
		   m.m_fMat[0+2*3]*(m.m_fMat[1+0*3]*m.m_fMat[2+1*3] - m.m_fMat[1+1*3]*m.m_fMat[2+0*3]);
}

const Mat33	Transpose( const Mat33 &m )
{
	Mat33 r;

	r.m_fMat[0+0*3] = m.m_fMat[0+0*3];
	r.m_fMat[1+0*3] = m.m_fMat[0+1*3];
	r.m_fMat[2+0*3] = m.m_fMat[0+2*3];

	r.m_fMat[0+1*3] = m.m_fMat[1+0*3];
	r.m_fMat[1+1*3] = m.m_fMat[1+1*3];
	r.m_fMat[2+1*3] = m.m_fMat[1+2*3];

	r.m_fMat[0+2*3] = m.m_fMat[2+0*3];
	r.m_fMat[1+2*3] = m.m_fMat[2+1*3];
	r.m_fMat[2+2*3] = m.m_fMat[2+2*3];

	return r;
}



void		LoadIdentity(Mat33 * pM)
{
	pM->m_fMat[0+0*3] = 1;
	pM->m_fMat[1+0*3] = 0;
	pM->m_fMat[2+0*3] = 0;
	pM->m_fMat[0+1*3] = 0;
	pM->m_fMat[1+1*3] = 1;
	pM->m_fMat[2+1*3] = 0;
	pM->m_fMat[0+2*3] = 0;
	pM->m_fMat[1+2*3] = 0;
	pM->m_fMat[2+2*3] = 1;
}

void		LoadRotation(Mat33 * pM, const Quat &Q )
{
	float tx  = 2*Q.V.x;
	float ty  = 2*Q.V.y;
	float tz  = 2*Q.V.z;
	float twx = tx*Q.s;
	float twy = ty*Q.s;
	float twz = tz*Q.s;
	float txx = tx*Q.V.x;
	float txy = ty*Q.V.x;
	float txz = tz*Q.V.x;
	float tyy = ty*Q.V.y;
	float tyz = tz*Q.V.y;
	float tzz = tz*Q.V.z;

	pM->m_fMat[0+0*3] = 1.0f-tyy-tzz;
	pM->m_fMat[0+1*3] = txy-twz;
	pM->m_fMat[0+2*3] = txz+twy;
	pM->m_fMat[1+0*3] = txy+twz;
	pM->m_fMat[1+1*3] = 1.0f-txx-tzz;
	pM->m_fMat[1+2*3] = tyz-twx;
	pM->m_fMat[2+0*3] = txz-twy;
	pM->m_fMat[2+1*3] = tyz+twx;
	pM->m_fMat[2+2*3] = 1.0f-txx-tyy;
}

// YXZ * p	(Z first)
void		LoadRotation(Mat33 * pM, const float fX, const float fY, const float fZ )
{
	float fCx = (float) cosf(fX), fSx = (float) sinf(fX);
	float fCy = (float) cosf(fY), fSy = (float) sinf(fY);
	float fCz = (float) cosf(fZ), fSz = (float) sinf(fZ);

	pM->m_fMat[0+0*3] = fSz*fSx*fSy + fCz*fCy;
	pM->m_fMat[1+0*3] = fSz*fCx;
	pM->m_fMat[2+0*3] = fSz*fSx*fCy - fCz*fSy;

	pM->m_fMat[0+1*3] = fCz*fSx*fSy - fSz*fCy;
	pM->m_fMat[1+1*3] = fCz*fCx;
	pM->m_fMat[2+1*3] = fCz*fSx*fCy + fSz*fSy;

	pM->m_fMat[0+2*3] = fCx*fSy;
	pM->m_fMat[1+2*3] = -fSx;
	pM->m_fMat[2+2*3] = fCx*fCy;
}

void		LoadRotationAxisAngle(Mat33 * pM, const Vec3 &v, const float fAngle )
{
	float C = (float) cosf(fAngle), S = (float) sinf(fAngle);
	float x2 = v.x*v.x, y2 = v.y*v.y, z2 = v.z*v.z;
	float xs = v.x*S, ys = v.y*S, zs = v.z*S;
	float xyOneMinusC = v.x*v.y*(1.0f-C);
	float zxOneMinusC = v.z*v.x*(1.0f-C);
	float yzOneMinusC = v.y*v.z*(1.0f-C);

	pM->m_fMat[0+0*3] = x2 + C*(1.0f-x2);
	pM->m_fMat[1+0*3] = xyOneMinusC + zs;
	pM->m_fMat[2+0*3] = zxOneMinusC - ys;
	
	pM->m_fMat[0+1*3] = xyOneMinusC - zs;
	pM->m_fMat[1+1*3] = y2 + C*(1.0f-y2);
	pM->m_fMat[2+1*3] = yzOneMinusC + xs;
	
	pM->m_fMat[0+2*3] = zxOneMinusC + ys;
	pM->m_fMat[1+2*3] = yzOneMinusC - xs;
	pM->m_fMat[2+2*3] = z2 + C*(1.0f-z2);
}




void		SetRow(Mat33 * pM, int iRow, const Vec3 &v )
{
	pM->m_fMat[iRow+0*3] = v.x;
	pM->m_fMat[iRow+1*3] = v.y;
	pM->m_fMat[iRow+2*3] = v.z;
}

const Vec3	GetRow(const Mat33 &m, int iRow )
{
	Vec3 v;

	v.x = m.m_fMat[iRow+0*3];
	v.y = m.m_fMat[iRow+1*3];
	v.z = m.m_fMat[iRow+2*3];

	return v;
}

void		SetColumn(Mat33 * pM, int iColumn, const Vec3 &v )
{
	pM->m_fMat[0+iColumn*3] = v.x;
	pM->m_fMat[1+iColumn*3] = v.y;
	pM->m_fMat[2+iColumn*3] = v.z;
}

const Vec3	GetColumn(const Mat33 &m, int iColumn )
{
	Vec3 v;

	v.x = m.m_fMat[0+iColumn*3];
	v.y = m.m_fMat[1+iColumn*3];
	v.z = m.m_fMat[2+iColumn*3];

	return v;
}



Mat33::Mat33( const Mat33 &m )
{
	m_fMat[0+0*3] = m.m_fMat[0+0*3];
	m_fMat[1+0*3] = m.m_fMat[1+0*3];
	m_fMat[2+0*3] = m.m_fMat[2+0*3];
	m_fMat[0+1*3] = m.m_fMat[0+1*3];
	m_fMat[1+1*3] = m.m_fMat[1+1*3];
	m_fMat[2+1*3] = m.m_fMat[2+1*3];
	m_fMat[0+2*3] = m.m_fMat[0+2*3];
	m_fMat[1+2*3] = m.m_fMat[1+2*3];
	m_fMat[2+2*3] = m.m_fMat[2+2*3];
}
