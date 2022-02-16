#include "mat44.h"
#include "vec4.h"
#include "quaternion.h"
#include <math.h>




const Vec4	operator *( const Mat44 &m, const Vec4 &v )
{
	Vec4 r;

	r.x = m.m_fMat[0+0*4]*v.x + m.m_fMat[0+1*4]*v.y + m.m_fMat[0+2*4]*v.z + m.m_fMat[0+3*4]*v.w;
	r.y = m.m_fMat[1+0*4]*v.x + m.m_fMat[1+1*4]*v.y + m.m_fMat[1+2*4]*v.z + m.m_fMat[1+3*4]*v.w;
	r.z = m.m_fMat[2+0*4]*v.x + m.m_fMat[2+1*4]*v.y + m.m_fMat[2+2*4]*v.z + m.m_fMat[2+3*4]*v.w;
	r.w = m.m_fMat[3+0*4]*v.x + m.m_fMat[3+1*4]*v.y + m.m_fMat[3+2*4]*v.z + m.m_fMat[3+3*4]*v.w;

	return r;
}

const Mat44	operator *( const Mat44 &m1, const Mat44 &m2 )
{
	Mat44 r;

	for(int iCol = 0; iCol < 4; iCol++)
	{
		r.m_fMat[0+iCol*4] = m1.m_fMat[0+0*4]*m2.m_fMat[0+iCol*4] + m1.m_fMat[0+1*4]*m2.m_fMat[1+iCol*4] + m1.m_fMat[0+2*4]*m2.m_fMat[2+iCol*4] + m1.m_fMat[0+3*4]*m2.m_fMat[3+iCol*4];
		r.m_fMat[1+iCol*4] = m1.m_fMat[1+0*4]*m2.m_fMat[0+iCol*4] + m1.m_fMat[1+1*4]*m2.m_fMat[1+iCol*4] + m1.m_fMat[1+2*4]*m2.m_fMat[2+iCol*4] + m1.m_fMat[1+3*4]*m2.m_fMat[3+iCol*4];
		r.m_fMat[2+iCol*4] = m1.m_fMat[2+0*4]*m2.m_fMat[0+iCol*4] + m1.m_fMat[2+1*4]*m2.m_fMat[1+iCol*4] + m1.m_fMat[2+2*4]*m2.m_fMat[2+iCol*4] + m1.m_fMat[2+3*4]*m2.m_fMat[3+iCol*4];
		r.m_fMat[3+iCol*4] = m1.m_fMat[3+0*4]*m2.m_fMat[0+iCol*4] + m1.m_fMat[3+1*4]*m2.m_fMat[1+iCol*4] + m1.m_fMat[3+2*4]*m2.m_fMat[2+iCol*4] + m1.m_fMat[3+3*4]*m2.m_fMat[3+iCol*4];
	}

	return r;
}

Mat44&		operator *=( Mat44 &m1, const Mat44 &m2 )
{
	m1 = m2 * m1;		// We add m2 to the heap of transformations

	return m1;
}

const Mat44 operator ~(const Mat44 &m)
{
	Mat44 r;

	float f22Det1  = m.m_fMat[2+2*4]*m.m_fMat[3+3*4] - m.m_fMat[2+3*4]*m.m_fMat[3+2*4];
	float f22Det2  = m.m_fMat[2+1*4]*m.m_fMat[3+3*4] - m.m_fMat[2+3*4]*m.m_fMat[3+1*4];
	float f22Det3  = m.m_fMat[2+1*4]*m.m_fMat[3+2*4] - m.m_fMat[2+2*4]*m.m_fMat[3+1*4];
	float f22Det4  = m.m_fMat[2+0*4]*m.m_fMat[3+3*4] - m.m_fMat[2+3*4]*m.m_fMat[3+0*4];
	float f22Det5  = m.m_fMat[2+0*4]*m.m_fMat[3+2*4] - m.m_fMat[2+2*4]*m.m_fMat[3+0*4];
	float f22Det6  = m.m_fMat[2+0*4]*m.m_fMat[3+1*4] - m.m_fMat[2+1*4]*m.m_fMat[3+0*4];
	float f22Det7  = m.m_fMat[1+2*4]*m.m_fMat[3+3*4] - m.m_fMat[1+3*4]*m.m_fMat[3+2*4];
	float f22Det8  = m.m_fMat[1+1*4]*m.m_fMat[3+3*4] - m.m_fMat[1+3*4]*m.m_fMat[3+1*4]; 
	float f22Det9  = m.m_fMat[1+1*4]*m.m_fMat[3+2*4] - m.m_fMat[1+2*4]*m.m_fMat[3+1*4];
	float f22Det10 = m.m_fMat[1+2*4]*m.m_fMat[2+3*4] - m.m_fMat[1+3*4]*m.m_fMat[2+2*4];
	float f22Det11 = m.m_fMat[1+1*4]*m.m_fMat[2+3*4] - m.m_fMat[1+3*4]*m.m_fMat[2+1*4];
	float f22Det12 = m.m_fMat[1+1*4]*m.m_fMat[2+2*4] - m.m_fMat[1+2*4]*m.m_fMat[2+1*4];
	float f22Det13 = m.m_fMat[1+0*4]*m.m_fMat[3+3*4] - m.m_fMat[1+3*4]*m.m_fMat[3+0*4];
	float f22Det14 = m.m_fMat[1+0*4]*m.m_fMat[3+2*4] - m.m_fMat[1+2*4]*m.m_fMat[3+0*4];
	float f22Det15 = m.m_fMat[1+0*4]*m.m_fMat[2+3*4] - m.m_fMat[1+3*4]*m.m_fMat[2+0*4];
	float f22Det16 = m.m_fMat[1+0*4]*m.m_fMat[2+2*4] - m.m_fMat[1+2*4]*m.m_fMat[2+0*4];
	float f22Det17 = m.m_fMat[1+0*4]*m.m_fMat[3+1*4] - m.m_fMat[1+1*4]*m.m_fMat[3+0*4];	
	float f22Det18 = m.m_fMat[1+0*4]*m.m_fMat[2+1*4] - m.m_fMat[1+1*4]*m.m_fMat[2+0*4];

	float fFirst33Det  = m.m_fMat[1+1*4]*f22Det1 - m.m_fMat[1+2*4]*f22Det2 + m.m_fMat[1+3*4]*f22Det3;	
	float fSec33Det    = m.m_fMat[1+0*4]*f22Det1 - m.m_fMat[1+2*4]*f22Det4 + m.m_fMat[1+3*4]*f22Det5;	
	float fThird33Det  = m.m_fMat[1+0*4]*f22Det2 - m.m_fMat[1+1*4]*f22Det4 + m.m_fMat[1+3*4]*f22Det6;
	float fFourth33Det = m.m_fMat[1+0*4]*f22Det3 - m.m_fMat[1+1*4]*f22Det5 + m.m_fMat[1+2*4]*f22Det6;

	float fDet44 = m.m_fMat[0+0*4]*fFirst33Det - m.m_fMat[0+1*4]*fSec33Det + m.m_fMat[0+2*4]*fThird33Det - m.m_fMat[0+3*4]*fFourth33Det;

	float s = 1.0f / fDet44;

	r.m_fMat[0+0*4] = s * fFirst33Det;
	r.m_fMat[0+1*4] = -s * ( m.m_fMat[0+1*4]*f22Det1 - m.m_fMat[0+2*4]*f22Det2 + m.m_fMat[0+3*4]*f22Det3 );
	r.m_fMat[0+2*4] = s * ( m.m_fMat[0+1*4]*f22Det7 - m.m_fMat[0+2*4]*f22Det8 + m.m_fMat[0+3*4]*f22Det9 );	
	r.m_fMat[0+3*4] = -s * ( m.m_fMat[0+1*4]*f22Det10 - m.m_fMat[0+2*4]*f22Det11 + m.m_fMat[0+3*4]*f22Det12 );

	r.m_fMat[1+0*4] = -s * fSec33Det;
	r.m_fMat[1+1*4] = s * ( m.m_fMat[0+0*4]*f22Det1 - m.m_fMat[0+2*4]*f22Det4 + m.m_fMat[0+3*4]*f22Det5 );
	r.m_fMat[1+2*4] = -s * ( m.m_fMat[0+0*4]*f22Det7 - m.m_fMat[0+2*4]*f22Det13 + m.m_fMat[0+3*4]*f22Det14 );
	r.m_fMat[1+3*4] = s * ( m.m_fMat[0+0*4]*f22Det10 - m.m_fMat[0+2*4]*f22Det15 + m.m_fMat[0+3*4]*f22Det16 );

	r.m_fMat[2+0*4] = s * fThird33Det;
	r.m_fMat[2+1*4] = -s * ( m.m_fMat[0+0*4]*f22Det2 - m.m_fMat[0+1*4]*f22Det4 + m.m_fMat[0+3*4]*f22Det6 );	
	r.m_fMat[2+2*4] = s * ( m.m_fMat[0+0*4]*f22Det8 - m.m_fMat[0+1*4]*f22Det13 + m.m_fMat[0+3*4]*f22Det17 );
	r.m_fMat[2+3*4] = -s * ( m.m_fMat[0+0*4]*f22Det11 - m.m_fMat[0+1*4]*f22Det15 + m.m_fMat[0+3*4]*f22Det18 );

	r.m_fMat[3+0*4] = -s * fFourth33Det;
	r.m_fMat[3+1*4] = s * ( m.m_fMat[0+0*4]*f22Det3 - m.m_fMat[0+1*4]*f22Det5 + m.m_fMat[0+2*4]*f22Det6 );
	r.m_fMat[3+2*4] = -s * ( m.m_fMat[0+0*4]*f22Det9 - m.m_fMat[0+1*4]*f22Det14 + m.m_fMat[0+2*4]*f22Det17 );
	r.m_fMat[3+3*4] = s * ( m.m_fMat[0+0*4]*f22Det12 - m.m_fMat[0+1*4]*f22Det16 + m.m_fMat[0+2*4]*f22Det18 );

	return r;
}



float Determinant(const Mat44 &m)
{
	float f22Det1  = m.m_fMat[2+2*4]*m.m_fMat[3+3*4] - m.m_fMat[2+3*4]*m.m_fMat[3+2*4];
	float f22Det2  = m.m_fMat[2+1*4]*m.m_fMat[3+3*4] - m.m_fMat[2+3*4]*m.m_fMat[3+1*4];
	float f22Det3  = m.m_fMat[2+1*4]*m.m_fMat[3+2*4] - m.m_fMat[2+2*4]*m.m_fMat[3+1*4];
	float f22Det4  = m.m_fMat[2+0*4]*m.m_fMat[3+3*4] - m.m_fMat[2+3*4]*m.m_fMat[3+0*4];
	float f22Det5  = m.m_fMat[2+0*4]*m.m_fMat[3+2*4] - m.m_fMat[2+2*4]*m.m_fMat[3+0*4];
	float f22Det6  = m.m_fMat[2+0*4]*m.m_fMat[3+1*4] - m.m_fMat[2+1*4]*m.m_fMat[3+0*4];

	float fFirst33Det  = m.m_fMat[1+1*4]*f22Det1 - m.m_fMat[1+2*4]*f22Det2 + m.m_fMat[1+3*4]*f22Det3;
	float fSec33Det    = m.m_fMat[1+0*4]*f22Det1 - m.m_fMat[1+2*4]*f22Det4 + m.m_fMat[1+3*4]*f22Det5;	
	float fThird33Det  = m.m_fMat[1+0*4]*f22Det2 - m.m_fMat[1+1*4]*f22Det4 + m.m_fMat[1+3*4]*f22Det6;
	float fFourth33Det = m.m_fMat[1+0*4]*f22Det3 - m.m_fMat[1+1*4]*f22Det5 + m.m_fMat[1+2*4]*f22Det6;


	return m.m_fMat[0+0*4]*fFirst33Det - m.m_fMat[0+1*4]*fSec33Det + m.m_fMat[0+2*4]*fThird33Det - m.m_fMat[0+3*4]*fFourth33Det; 
}

const Mat44	Transpose( const Mat44 &m )
{
	Mat44 r;

	r.m_fMat[0+0*4] = m.m_fMat[0+0*4];
	r.m_fMat[1+0*4] = m.m_fMat[0+1*4];
	r.m_fMat[2+0*4] = m.m_fMat[0+2*4];
	r.m_fMat[3+0*4] = m.m_fMat[0+3*4];

	r.m_fMat[0+1*4] = m.m_fMat[1+0*4];
	r.m_fMat[1+1*4] = m.m_fMat[1+1*4];
	r.m_fMat[2+1*4] = m.m_fMat[1+2*4];
	r.m_fMat[3+1*4] = m.m_fMat[1+3*4];

	r.m_fMat[0+2*4] = m.m_fMat[2+0*4];
	r.m_fMat[1+2*4] = m.m_fMat[2+1*4];
	r.m_fMat[2+2*4] = m.m_fMat[2+2*4];
	r.m_fMat[3+2*4] = m.m_fMat[2+3*4];

	r.m_fMat[0+3*4] = m.m_fMat[3+0*4];
	r.m_fMat[1+3*4] = m.m_fMat[3+1*4];
	r.m_fMat[2+3*4] = m.m_fMat[3+2*4];
	r.m_fMat[3+3*4] = m.m_fMat[3+3*4];

	return r;
}



void		LoadIdentity(Mat44 * pM)
{
	pM->m_fMat[0+0*4] = 1;
	pM->m_fMat[1+0*4] = 0;
	pM->m_fMat[2+0*4] = 0;
	pM->m_fMat[3+0*4] = 0;
	pM->m_fMat[0+1*4] = 0;
	pM->m_fMat[1+1*4] = 1;
	pM->m_fMat[2+1*4] = 0;
	pM->m_fMat[3+1*4] = 0;
	pM->m_fMat[0+2*4] = 0;
	pM->m_fMat[1+2*4] = 0;
	pM->m_fMat[2+2*4] = 1;
	pM->m_fMat[3+2*4] = 0;
	pM->m_fMat[0+3*4] = 0;
	pM->m_fMat[1+3*4] = 0;
	pM->m_fMat[2+3*4] = 0;
	pM->m_fMat[3+3*4] = 1;
}

void		LoadRotation(Mat44 * pM, const Quat &Q )
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

	pM->m_fMat[0+0*4] = 1.0f-tyy-tzz;
	pM->m_fMat[0+1*4] = txy-twz;
	pM->m_fMat[0+2*4] = txz+twy;
	pM->m_fMat[0+3*4] = 0;
	pM->m_fMat[1+0*4] = txy+twz;
	pM->m_fMat[1+1*4] = 1.0f-txx-tzz;
	pM->m_fMat[1+2*4] = tyz-twx;
	pM->m_fMat[1+3*4] = 0;
	pM->m_fMat[2+0*4] = txz-twy;
	pM->m_fMat[2+1*4] = tyz+twx;
	pM->m_fMat[2+2*4] = 1.0f-txx-tyy;
	pM->m_fMat[2+3*4] = 0;
	pM->m_fMat[3+0*4] = 0;
	pM->m_fMat[3+1*4] = 0;
	pM->m_fMat[3+2*4] = 0;
	pM->m_fMat[3+3*4] = 1.0f;
}

// YXZ * p	(Z first)
void		LoadRotation(Mat44 * pM, const float fX, const float fY, const float fZ )
{
	float fCx = (float) cosf(fX), fSx = (float) sinf(fX);
	float fCy = (float) cosf(fY), fSy = (float) sinf(fY);
	float fCz = (float) cosf(fZ), fSz = (float) sinf(fZ);

	pM->m_fMat[0+0*4] = fSz*fSx*fSy + fCz*fCy;
	pM->m_fMat[1+0*4] = fSz*fCx;
	pM->m_fMat[2+0*4] = fSz*fSx*fCy - fCz*fSy;
	pM->m_fMat[3+0*4] = 0;	

	pM->m_fMat[0+1*4] = fCz*fSx*fSy - fSz*fCy;
	pM->m_fMat[1+1*4] = fCz*fCx;
	pM->m_fMat[2+1*4] = fCz*fSx*fCy + fSz*fSy;
	pM->m_fMat[3+1*4] = 0;

	pM->m_fMat[0+2*4] = fCx*fSy;
	pM->m_fMat[1+2*4] = -fSx;
	pM->m_fMat[2+2*4] = fCx*fCy;
	pM->m_fMat[3+2*4] = 0;

	pM->m_fMat[0+3*4] = 0;
	pM->m_fMat[1+3*4] = 0;
	pM->m_fMat[2+3*4] = 0;
	pM->m_fMat[3+3*4] = 1.0f;
}

void		LoadRotationAxisAngle(Mat44 * pM, const Vec3 &v, const float fAngle )
{
	float C = (float) cosf(fAngle), S = (float) sinf(fAngle);
	float x2 = v.x*v.x, y2 = v.y*v.y, z2 = v.z*v.z;
	float xs = v.x*S, ys = v.y*S, zs = v.z*S;
	float xyOneMinusC = v.x*v.y*(1.0f-C);
	float zxOneMinusC = v.z*v.x*(1.0f-C);
	float yzOneMinusC = v.y*v.z*(1.0f-C);

	pM->m_fMat[0+0*4] = x2 + C*(1.0f-x2);
	pM->m_fMat[1+0*4] = xyOneMinusC + zs;
	pM->m_fMat[2+0*4] = zxOneMinusC - ys;
	pM->m_fMat[3+0*4] = 0;
	
	pM->m_fMat[0+1*4] = xyOneMinusC - zs;
	pM->m_fMat[1+1*4] = y2 + C*(1.0f-y2);
	pM->m_fMat[2+1*4] = yzOneMinusC + xs;
	pM->m_fMat[3+1*4] = 0;
	
	pM->m_fMat[0+2*4] = zxOneMinusC + ys;
	pM->m_fMat[1+2*4] = yzOneMinusC - xs;
	pM->m_fMat[2+2*4] = z2 + C*(1.0f-z2);
	pM->m_fMat[3+2*4] = 0;

	pM->m_fMat[0+3*4] = 0;
	pM->m_fMat[1+3*4] = 0;
	pM->m_fMat[2+3*4] = 0;
	pM->m_fMat[3+3*4] = 1.0f;
}




void		SetRow(Mat44 * pM, int iRow, const Vec4 &v )
{
	pM->m_fMat[iRow+0*4] = v.x;
	pM->m_fMat[iRow+1*4] = v.y;
	pM->m_fMat[iRow+2*4] = v.z;
	pM->m_fMat[iRow+3*4] = v.w;
}

const Vec4	GetRow(const Mat44 &m, int iRow )
{
	Vec4 v;

	v.x = m.m_fMat[iRow+0*4];
	v.y = m.m_fMat[iRow+1*4];
	v.z = m.m_fMat[iRow+2*4];
	v.w = m.m_fMat[iRow+3*4];

	return v;
}

void		SetColumn(Mat44 * pM, int iColumn, const Vec4 &v )
{
	pM->m_fMat[0+iColumn*4] = v.x;
	pM->m_fMat[1+iColumn*4] = v.y;
	pM->m_fMat[2+iColumn*4] = v.z;
	pM->m_fMat[3+iColumn*4] = v.w;
}

const Vec4	GetColumn(const Mat44 &m, int iColumn )
{
	Vec4 v;

	v.x = m.m_fMat[0+iColumn*4];
	v.y = m.m_fMat[1+iColumn*4];
	v.z = m.m_fMat[2+iColumn*4];
	v.w = m.m_fMat[3+iColumn*4];

	return v;
}



Mat44::Mat44( const Mat44 &m )
{
	m_fMat[0+0*4] = m.m_fMat[0+0*4];
	m_fMat[1+0*4] = m.m_fMat[1+0*4];
	m_fMat[2+0*4] = m.m_fMat[2+0*4];
	m_fMat[3+0*4] = m.m_fMat[3+0*4];
	m_fMat[0+1*4] = m.m_fMat[0+1*4];
	m_fMat[1+1*4] = m.m_fMat[1+1*4];
	m_fMat[2+1*4] = m.m_fMat[2+1*4];
	m_fMat[3+1*4] = m.m_fMat[3+1*4];
	m_fMat[0+2*4] = m.m_fMat[0+2*4];
	m_fMat[1+2*4] = m.m_fMat[1+2*4];
	m_fMat[2+2*4] = m.m_fMat[2+2*4];
	m_fMat[3+2*4] = m.m_fMat[3+2*4];
	m_fMat[0+3*4] = m.m_fMat[0+3*4];
	m_fMat[1+3*4] = m.m_fMat[1+3*4];
	m_fMat[2+3*4] = m.m_fMat[2+3*4];
	m_fMat[3+3*4] = m.m_fMat[3+3*4];
}
