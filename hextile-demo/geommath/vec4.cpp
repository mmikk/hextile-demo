#include "vec4.h"
#include <math.h>
#include "vec3.h"



const Vec4		operator -( const Vec4 &v1, const Vec4 &v2 )
{
	Vec4 vRes;

	vRes.x = v1.x - v2.x;
	vRes.y = v1.y - v2.y;
	vRes.z = v1.z - v2.z;
	vRes.w = v1.w - v2.w;

	return vRes;
}

const Vec4		operator -( const Vec4 &v )
{
	Vec4 vRes;

	vRes.x = -v.x;
	vRes.y = -v.y;
	vRes.z = -v.z;
	vRes.w = -v.w;

	return vRes;
}

const Vec4		operator +( const Vec4 &v1, const Vec4 &v2 )
{
	Vec4 vRes;

	vRes.x = v1.x + v2.x;
	vRes.y = v1.y + v2.y;
	vRes.z = v1.z + v2.z;
	vRes.w = v1.w + v2.w;

	return vRes;
}

Vec4&			operator +=(Vec4 &v1, const Vec4 &v2)
{
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	v1.w += v2.w;

	return v1;
}

Vec4&			operator -=(Vec4 &v1, const Vec4 &v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	v1.w -= v2.w;

	return v1;
}

bool			operator ==( const Vec4 &v1, const Vec4 &v2 )
{
	return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w);
}

bool			operator !=( const Vec4 &v1, const Vec4 &v2 )
{
	return !(v1==v2);
}


const float		operator *( const Vec4 &v1, const Vec4 &v2)
{
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
}

const Vec4		operator *(const float fS, const Vec4 &v)
{
	Vec4 vRes;

	vRes.x = fS * v.x;
	vRes.y = fS * v.y;
	vRes.z = fS * v.z;
	vRes.w = fS * v.w;

	return vRes;
}

const Vec4		operator *(const Vec4 &v, const float fS)
{
	return fS * v;
}

Vec4&			operator *=(Vec4 &v, const float fS )
{
	v.x *= fS;
	v.y *= fS;
	v.z *= fS;
	v.w *= fS;

	return v;
}



const Vec4		Cross(const Vec4 &v1, const Vec4 &v2, const Vec4 &v3)
{
	Vec4 v;		   // Result Vector
	float  fA, fB, fC, fD, fE, fF;       // Intermediate Values
    

    // Calculate intermediate values.	
    fA = (v2.x * v3.y) - (v2.y * v3.x);
    fB = (v2.x * v3.z) - (v2.z * v3.x);
    fC = (v2.x * v3.w) - (v2.w * v3.x);
    fD = (v2.y * v3.z) - (v2.z * v3.y);
    fE = (v2.y * v3.w) - (v2.w * v3.y);
    fF = (v2.z * v3.w) - (v2.w * v3.z);

    // Calculate the result-vector components.
    v.x =   (v1.y * fF) - (v1.z * fE) + (v1.w * fD);
    v.y = - (v1.x * fF) + (v1.z * fC) - (v1.w * fB);
    v.z =   (v1.x * fE) - (v1.y * fC) + (v1.w * fA);
    v.w = - (v1.x * fD) + (v1.y * fB) - (v1.z * fA);

	return v;
}

float			LengthSquared( const Vec4 &v )
{
	return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;
}

float			Length( const Vec4 &v )
{
	return (float) sqrt(LengthSquared(v));
}

const Vec4		Normalize( const Vec4 &v )
{
	return (1 / Length(v)) * v;
}



Vec4::operator Vec3() const
{
	Vec3 v;

	float fInvW = 1 / w;

	v.x = fInvW * x;
	v.y = fInvW * y;
	v.z = fInvW * z;

	return v;
}
