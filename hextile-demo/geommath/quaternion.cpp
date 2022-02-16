#include "quaternion.h"
#include "mat33.h"
#include <math.h>



#define	M_EPSILON	0.001f


Quat&		operator +=(Quat &Qa, const Quat &Qb)
{
	Qa.s = Qa.s + Qb.s;
	Qa.V = Qa.V + Qb.V;

	return Qa;
}

Quat&		operator -=(Quat &Qa, const Quat &Qb)
{
	Qa.s = Qa.s - Qb.s;
	Qa.V = Qa.V - Qb.V;

	return Qa;
}

Quat&		operator *=(Quat &Qa, const Quat &Qb)
{
	Qa.s = Qa.s * Qb.s - Qa.V*Qb.V;
	Qa.V = Cross(Qa.V, Qb.V) + Qa.s * Qb.V + Qb.s * Qa.V;

	return Qa;
}

Quat&		operator *=(Quat &Q, float r)
{
	Q.V *= r;
	Q.s *= r;

	return Q;
}

Quat&		SetIdentity(Quat &Q)
{
	Q.s = 1.0f;
	Q.V.x = 0.0f;
	Q.V.y = 0.0f;
	Q.V.z = 0.0f;

	return Q;
}




const Quat	operator +(const Quat &Qa, const Quat &Qb)
{
	return Quat( Qa.V + Qb.V, Qa.s + Qb.s );
}

const Quat	operator -(const Quat &Qa, const Quat &Qb)
{
	return Quat( Qa.V - Qb.V, Qa.s - Qb.s );
}

const Quat	operator -(const Quat &Q)
{
	return Quat( -Q.V, -Q.s );
}

const Quat	operator *(const Quat &Qa, const Quat &Qb)
{
	return Quat( Cross(Qa.V, Qb.V) + Qa.s * Qb.V + Qb.s * Qa.V,  Qa.s * Qb.s - Qa.V*Qb.V );
}

const Quat	operator *(const Quat &Q, float r)
{
	return Quat( Q.V * r, Q.s * r );
}

const Quat	operator *(float r, const Quat &Q)
{
	return (Q * r);
}




const Quat	Normalize( const Quat &Q)
{
	float f = 1.0f / Norm(Q);

	return Q*f;
}

const Quat	GetInverse(const Quat &Q)
{
	float f = 1.0f / Norm(Q);

	return Quat( Q.V * (-f), Q.s * f );
}

const Quat	GetUnitInverse(const Quat &Q)
{
	return Quat( -Q.V, Q.s );
}

const Quat	AxisAngleToQuat(const Vec3 &AxisAngle)
{
	Quat Res;
	float fAngle = Length(AxisAngle);

	if(fabs(fAngle) < M_EPSILON )
	{
		SetIdentity(Res);
		return Res;
	}

    float fHalfAngle = 0.5f*fAngle;
    float sn = (float) sin(fHalfAngle);

    return Quat( AxisAngle * (sn / fAngle), (float) cos(fHalfAngle) );
}





float	Dot(const Quat &Qa, const Quat &Qb)
{
	return ( Qa.s * Qb.s + Qa.V*Qb.V );
}

float	Norm(const Quat &Q)
{
	return (float) sqrt( Q.s * Q.s + Q.V*Q.V );
}

const Quat	RotationToQuat(const Mat33 &m)
{
	// Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quat Calculus and Fast Animation".
	Quat Res;
	float trace = m.m_fMat[0+0*3]+m.m_fMat[1+1*3]+m.m_fMat[2+2*3];
	float root;

	if ( trace > 0.0f )
	{
		// |w| > 1/2, may as well choose w > 1/2
		root = (float) sqrtf(trace+1.0f);  // 2w
		Res.s = 0.5f*root;
		root = 0.5f/root;  // 1/(4w)
		Res.V.x = (m.m_fMat[2+1*3]-m.m_fMat[1+2*3])*root;
		Res.V.y = (m.m_fMat[0+2*3]-m.m_fMat[2+0*3])*root;
		Res.V.z = (m.m_fMat[1+0*3]-m.m_fMat[0+1*3])*root;
	}
	else
	{
		// |w| <= 1/2
		static int next[3] = { 1, 2, 0 };
		int i = 0;
		if ( m.m_fMat[1+1*3] > m.m_fMat[0+0*3] )
			i = 1;
		if ( m.m_fMat[2+2*3] > m.m_fMat[i+i*3] )
			i = 2;
		int j = next[i];
		int k = next[j];
		
		root = (float) sqrtf(m.m_fMat[i+i*3]-m.m_fMat[j+j*3]-m.m_fMat[k+k*3]+1.0f);
		float* quat[3] = { &(Res.V.x), &(Res.V.y), &(Res.V.z) };
		*quat[i] = 0.5f*root;
		root = 0.5f/root;
		Res.s = (m.m_fMat[k+j*3]-m.m_fMat[j+k*3])*root;
		*quat[j] = (m.m_fMat[j+i*3]+m.m_fMat[i+j*3])*root;
		*quat[k] = (m.m_fMat[k+i*3]+m.m_fMat[i+k*3])*root;
	}

	return Res;
}



const Quat Slerp(const Quat &Qa, const Quat &Qb, float t)
{
	Quat	Res;
	float omega, cosom, sinom, sclp, sclq;
	bool bFlip = false;		// Default value false
	
	// cosine theta = dot product of A and B 
	cosom = Qa.V.x*Qb.V.x + Qa.V.y*Qb.V.y + Qa.V.z*Qb.V.z + Qa.s*Qb.s;

	// if B is on opposite hemisphere from A, use -B instead 
	if (cosom < 0.0f)
    {
		cosom = -cosom;
		bFlip = true; // true
	}

	// if B is (within precision limits) the same as A,
    // just linear interpolate between A and B.
    // Can't do spins, since we don't know what direction to spin.
	if (1.0f - cosom < M_EPSILON)
	{
		sclp = 1.0f - t;
		sclq = t;
	}
	else // normal case
	{
		omega = (float) acos(cosom);
		sinom = (float) sin(omega);
		sclp = (float) sin((1.0f-t)*omega) / sinom;
		sclq = (float) sin(t*omega) / sinom;
	}

	if (bFlip == 1)
	{
		sclq = -sclq;
	}

	
	// interpolate 
	Res.V.x = sclp*Qa.V.x + sclq*Qb.V.x;
	Res.V.y = sclp*Qa.V.y + sclq*Qb.V.y;
	Res.V.z = sclp*Qa.V.z + sclq*Qb.V.z;
	Res.s   = sclp*Qa.s   + sclq*Qb.s;
	
	
	return Res;
}


// Very fast slerping, maybe we can use it later
/*const Quat Slerp2(const float t, const float angle, const float cs, const float invsin, const Quat &Qa, const Quat &Qb)
{
	float c0,c1,c2;		
	Quat	res;
	Quat tmpQ;
	if( cs<0 )
	{
		tmpQ=-Qb;
		c2=-cs;
	}
	else
	{
		tmpQ=Qb;
		c2=cs;
	}

	if(invsin == 0.0f)
	{
		res = Qa*(1.0f-t) + tmpQ*t;
		return res;
	}

	c1=invsin*sin(t*angle);
	c0=cos(t*angle)-c2*c1;
	res=Qa*c0 + tmpQ*c1;

	return res;
}*/


