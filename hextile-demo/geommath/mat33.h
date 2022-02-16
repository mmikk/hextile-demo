#ifndef __MAT33_H__
#define __MAT33_H__

struct Vec3;
struct Quat;

struct Mat33
{
	Mat33() {}
	Mat33( const Mat33 &m );


	float m_fMat[3*3];
};


const Vec3	operator *( const Mat33 &m, const Vec3 &v );
const Mat33	operator *( const Mat33 &m1, const Mat33 &m2 );
Mat33&		operator *=( Mat33 &m1, const Mat33 &m2 );

const Mat33	operator ~( const Mat33 &m );
float		Determinant( const Mat33 &m );
const Mat33	Transpose( const Mat33 &m );

void		LoadIdentity(Mat33 * pM);
void		LoadRotation(Mat33 * pM, const Quat &Q );
void		LoadRotation(Mat33 * pM, const float fX, const float fY, const float fZ );
void		LoadRotationAxisAngle(Mat33 * pM, const Vec3 &v, const float fAngle );

void		SetRow(Mat33 * pM, int iRow, const Vec3 &v );
const Vec3	GetRow(const Mat33 &m, int iRow );
void		SetColumn(Mat33 * pM, int iColumn, const Vec3 &v );
const Vec3	GetColumn(const Mat33 &m, int iColumn );



#endif
