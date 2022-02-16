#ifndef __SHADERBASE_H__
#define __SHADERBASE_H__


#ifndef __cplusplus
#define Vec2		float2
#define Vec3		float3
#define Vec4		float4
#define Mat44		float4x4
#define unistruct	cbuffer
#define hbool		bool

#define _CB_REGSLOT(x) 		: register(x)
#define _QALIGN(x)	 		: packoffset(c0);

#else
#include <stddef.h>

#define ATTR_OFFS		offsetof

#define hbool		unsigned int
#define unistruct	struct
#define _CB_REGSLOT(x)
#define _QALIGN(x)

struct Vec2
{
	Vec2( const Vec2 &v ) : x(v.x), y(v.y) {}
	Vec2( float fX, float fY ) : x(fX), y(fY) {}
	Vec2() {}

	float x, y;
};


#endif


#endif