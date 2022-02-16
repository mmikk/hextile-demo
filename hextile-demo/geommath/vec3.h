#ifndef __VEC3_H__
#define __VEC3_H__


struct Vec4;

struct Vec3
{
	Vec3( const Vec3 &v ) : x(v.x), y(v.y), z(v.z) {}
	Vec3( float fX, float fY, float fZ ) : x(fX), y(fY), z(fZ) {}
	Vec3() {}

	operator Vec4() const;


	float x, y, z;
};


const Vec3		operator -( const Vec3 &v1, const Vec3 &v2 );
const Vec3		operator -( const Vec3 &v );
const Vec3		operator +( const Vec3 &v1, const Vec3 &v2 );
Vec3&			operator +=(Vec3 &v1, const Vec3 &v2);
Vec3&			operator -=(Vec3 &v1, const Vec3 &v2);
bool			operator ==( const Vec3 &v1, const Vec3 &v2 );
bool			operator !=( const Vec3 &v1, const Vec3 &v2 );

const float		operator *( const Vec3 &v1, const Vec3 &v2);
const Vec3		operator *(const float fS, const Vec3 &v);
const Vec3		operator *(const Vec3 &v, const float fS);
Vec3&			operator *=(Vec3 &v, const float fS );

const Vec3		Cross(const Vec3 &v1, const Vec3 &v2);
float			LengthSquared( const Vec3 &v );
float			Length( const Vec3 &v );
const Vec3		Normalize( const Vec3 &v );


#endif
