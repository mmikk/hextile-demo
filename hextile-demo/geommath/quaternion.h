#ifndef __QUATERNION_H__
#define __QUATERNION_H__

#include "vec3.h"


struct Mat33;

struct Quat
{
	Quat( const Vec3 &v, const float fS ) : V(v), s(fS) {}
	Quat( const Quat &Q ) : V(Q.V), s(Q.s) {}
	Quat() {}


	Vec3 V;
	float s;
};


Quat	&	operator +=(Quat &Qa, const Quat &Qb);
Quat	&	operator -=(Quat &Qa, const Quat &Qb);
Quat	&	operator *=(Quat &Qa, const Quat &Qb);
Quat	&	operator *=(Quat &Q, const float r);
Quat	&	SetIdentity(Quat &Q);


const Quat		operator +(const Quat &Qa, const Quat &Qb);
const Quat		operator -(const Quat &Qa, const Quat &Qb);
const Quat		operator -(const Quat &Q);
const Quat		operator *(const Quat &Qa, const Quat &Qb);
const Quat		operator *(float r, const Quat &Q);
const Quat		operator *(const Quat &Q, const float r);


const Quat		Normalize( const Quat &Q);
const Quat		GetInverse(const Quat &Q);
const Quat		GetUnitInverse(const Quat &Q);
const Quat		AxisAngleToQuat(const Vec3 &AxisAngle);

float		Dot(const Quat &Qa, const Quat &Qb);
float		Norm(const Quat &Q);
const Quat		RotationToQuat(const Mat33 &m);

const Quat Slerp(const Quat &Qa, const Quat &Qb, float f);



#endif
