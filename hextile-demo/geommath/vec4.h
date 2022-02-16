#ifndef __VEC4_H__
#define __VEC4_H__


struct Vec3;

struct Vec4
{
	Vec4( const Vec4 &v ) : x(v.x), y(v.y), z(v.z), w(v.w) {}
	Vec4( float fX, float fY, float fZ, float fW ) : x(fX), y(fY), z(fZ), w(fW) {}
	Vec4() {}

	operator Vec3() const;


	float x, y, z, w;
};


const Vec4		operator -( const Vec4 &v1, const Vec4 &v2 );
const Vec4		operator -( const Vec4 &v );
const Vec4		operator +( const Vec4 &v1, const Vec4 &v2 );
Vec4&			operator +=(Vec4 &v1, const Vec4 &v2);
Vec4&			operator -=(Vec4 &v1, const Vec4 &v2);
bool			operator ==( const Vec4 &v1, const Vec4 &v2 );
bool			operator !=( const Vec4 &v1, const Vec4 &v2 );

const float		operator *( const Vec4 &v1, const Vec4 &v2);
const Vec4		operator *(const float fS, const Vec4 &v);
const Vec4		operator *(const Vec4 &v, const float fS);
Vec4&			operator *=(Vec4 &v, const float fS );

const Vec4		Cross(const Vec4 &v1, const Vec4 &v2, const Vec4 &v3);
float			LengthSquared( const Vec4 &v );
float			Length( const Vec4 &v );
const Vec4		Normalize( const Vec4 &v );


#endif
