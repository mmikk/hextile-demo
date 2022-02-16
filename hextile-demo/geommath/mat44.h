#ifndef __MAT44_H__
#define __MAT44_H__

struct Vec3;
struct Vec4;
struct Quat;

struct Mat44
{
	Mat44() {}
	Mat44( const Mat44 &m );


	float m_fMat[4*4];
};


const Vec4	operator *( const Mat44 &m, const Vec4 &v );
const Mat44	operator *( const Mat44 &m1, const Mat44 &m2 );
Mat44&		operator *=( Mat44 &m1, const Mat44 &m2 );

const Mat44	operator ~( const Mat44 &m );
float		Determinant( const Mat44 &m );
const Mat44	Transpose( const Mat44 &m );

void		LoadIdentity(Mat44 * pM);
void		LoadRotation(Mat44 * pM, const Quat &Q );
void		LoadRotation(Mat44 * pM, const float fX, const float fY, const float fZ );
void		LoadRotationAxisAngle(Mat44 * pM, const Vec3 &v, const float fAngle );

void		SetRow(Mat44 * pM, int iRow, const Vec4 &v );
const Vec4	GetRow(const Mat44 &m, int iRow );
void		SetColumn(Mat44 * pM, int iColumn, const Vec4 &v );
const Vec4	GetColumn(const Mat44 &m, int iColumn );

#endif
