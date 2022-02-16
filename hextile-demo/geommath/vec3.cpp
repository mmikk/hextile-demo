#include "vec3.h"
#include <math.h>
#include "vec4.h"



const Vec3		operator -( const Vec3 &v1, const Vec3 &v2 )
{
	Vec3 vRes;

	vRes.x = v1.x - v2.x;
	vRes.y = v1.y - v2.y;
	vRes.z = v1.z - v2.z;

	return vRes;
}

const Vec3		operator -( const Vec3 &v )
{
	Vec3 vRes;

	vRes.x = -v.x;
	vRes.y = -v.y;
	vRes.z = -v.z;

	return vRes;
}

const Vec3		operator +( const Vec3 &v1, const Vec3 &v2 )
{
	Vec3 vRes;

	vRes.x = v1.x + v2.x;
	vRes.y = v1.y + v2.y;
	vRes.z = v1.z + v2.z;

	return vRes;
}

Vec3&			operator +=(Vec3 &v1, const Vec3 &v2)
{
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;

	return v1;
}

Vec3&			operator -=(Vec3 &v1, const Vec3 &v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;

	return v1;
}

bool			operator ==( const Vec3 &v1, const Vec3 &v2 )
{
	return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
}

bool			operator !=( const Vec3 &v1, const Vec3 &v2 )
{
	return !(v1==v2);
}


const float		operator *( const Vec3 &v1, const Vec3 &v2)
{
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

const Vec3		operator *(const float fS, const Vec3 &v)
{
	Vec3 vRes;

	vRes.x = fS * v.x;
	vRes.y = fS * v.y;
	vRes.z = fS * v.z;

	return vRes;
}

const Vec3		operator *(const Vec3 &v, const float fS)
{
	return fS * v;
}

Vec3&			operator *=(Vec3 &v, const float fS )
{
	v.x *= fS;
	v.y *= fS;
	v.z *= fS;

	return v;
}



const Vec3		Cross(const Vec3 &v1, const Vec3 &v2)
{
	Vec3 v;
	
	v.x = v1.y*v2.z - v2.y*v1.z;
	v.y = v1.z*v2.x - v2.z*v1.x;
	v.z = v1.x*v2.y - v2.x*v1.y;
	
	return v;
}

float			LengthSquared( const Vec3 &v )
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

float			Length( const Vec3 &v )
{
	return (float) sqrt(LengthSquared(v));
}

const Vec3		Normalize( const Vec3 &v )
{
	return (1 / Length(v)) * v;
}



Vec3::operator Vec4() const
{
	Vec4 v;

	v.x = x;
	v.y = y;
	v.z = z;
	v.w = 1;

	return v;
}
