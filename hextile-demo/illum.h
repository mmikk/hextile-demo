#ifndef __ILLUM_H__
#define __ILLUM_H__

/**
 *  Copyright (C) 2011 by Morten S. Mikkelsen
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */


/// IMPORTANT!!!! ///
/// IMPORTANT!!!! ///
// All these are implemented such that n_dot_l is built into the BRDF
// and thus the shader using them should NOT multiply by this factor.
// The diffuse term is also built into the BRDF.

// Another important observation is that most implementations of the Torrance-Sparrow
// or Cook-Torrance model neglect to deal with division by zero issues.
// Most often those that do don't do it well such that the right limit value is determined
// giving a smooth/continuous behavior.

// A clear derivation of the Torrance-Sparrow model is given in section 2.4
// of my paper --> http://jbit.net/~sparky/academic/mm_brdf.pdf
// Note the error in the cook-torrance model pointed out at the end of this section.
// Another important observation is that the bechmann distribution can be replaced
// with a normalized phong distribution (which is cheaper) using: float toNPhong(const float m)


#ifndef M_PI
	#define M_PI 3.1415926535897932384626433832795
#endif

float3 FixMyNormal(float3 vN, float3 vV);
float toBeckmannParam(const float n);
float toNPhong(const float m);

// Schlick's Fresnel approximation
float fresnelReflectance( float VdotH, float F0 )
{
	float base = 1-VdotH;
	float exponential = pow(base, 5.0);	// replace this by 3 muls for C/C++
	return saturate(exponential + F0 * (1 - exponential));
}

#define FLT_EPSILON     1.192092896e-07F        // smallest such that 1.0+FLT_EPSILON != 1.0
#define FLT_MAX         3.402823466e+38F        // max value
#define FLT_MIN         1.175494351e-38F        // min positive value






float VisibDivSmithGGX(float LdotN, float VdotN, float roughness)
{	
	float rough_sqr = roughness*roughness;
	
	float denom_v = VdotN + sqrt( lerp(VdotN*VdotN, 1.0, rough_sqr) );
	float denom_l = LdotN + sqrt( lerp(LdotN*LdotN, 1.0, rough_sqr) );

	return 4.0 / max(FLT_EPSILON, denom_l*denom_v);		// already divided by VdotN and LdotN
}




// this is the ggx model used in a Torrance-Sparrow formulation
float3 BRDF_ts_ggx(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float smoothness, float F0=0.04)
{
	// reflect hack when view vector is occluded
	// (doesn't seem to be needed)
	//vN = FixMyNormal(vN, vV);

	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// D is a surface distribution function and obeys:
	// D(vH)*HdotN is normalized (over half-spere)
	// Specifically, this is the ggx model
	float perceptual_roughness = 1.0-smoothness;
	float roughness = perceptual_roughness*perceptual_roughness;
	float k = roughness*roughness; 
	float denom = HdotN*HdotN * (k-1) + 1;
	denom = max(FLT_EPSILON, M_PI*denom*denom);
	const float D = k / denom; 

	// VisibDivSmithGGX visibility term divided by VdotN and LdotN
	const float fVdivDots = VisibDivSmithGGX(LdotN, VdotN, roughness);
	
	// Schlick's approximation
	const float fFres = fresnelReflectance(VdotH, F0);

	// torrance-sparrow:
	// (F * G * D) / (4 * LdotN * VdotN)
	// Division by VdotN and LdotN is done in VisibDivGGX()
	// outgoing radiance is determined by:
	// BRDF * LdotN * L()
	float fSpec = (fFres * fVdivDots * D) / 4;
	
	// sum up: diff + spec
	float3 res = LdotN * ((Cd/M_PI) + Cs * fSpec);
	return res;
}

// optional variant (self shadowing factor)
// vN is the shade normal (from a bump/normal map)
// vN2 represents the normalized interpolated vertex normal
float3 BRDF2_ts_ggx(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float smoothness, float F0=0.04)
{
	float3 res = BRDF_ts_ggx(vN, vL, vV, Cd, Cs, smoothness, F0);
	return saturate(4*dot(vL, vN2)) * res;
}





// The Torrance-Sparrow visibility factor, G,
// as described by Jim Blinn but divided by VdotN
// Note that this was carefully implemented such that division
// by zero problems and awkward discontinuities are avoided.
float VisibDiv(float LdotN, float VdotN, float VdotH, float HdotN)
{	
	// VdotH should never be zero. Only possible if
	// L and V end up in the same plane (unlikely).
	const float denom = max( VdotH, FLT_EPSILON );	
										
	float numL = min(VdotN, LdotN);
	const float numR = 2*HdotN;
	if((numL*numR)<=denom)	// min(x,1) = x
	{
		numL = numL == VdotN ? 1.0 : (LdotN / VdotN);	// VdotN is > 0 if this division is used
		return (numL*numR) / denom;
	}
	else					// min(x,1) = 1				this branch is taken when H and N are "close" (see fig. 3)
		return 1.0 / VdotN;
		// VdotN >= HdotN*VdotN >= HdotN*min(VdotN, LdotN) >= FLT_EPSILON/2
}


// this is a normalized Phong model used in the Torrance-Sparrow model
float3 BRDF_ts_nphong(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32, float F0=0.04)
{
	// reflect hack when view vector is occluded
	// (doesn't seem to be needed)
	//vN = FixMyNormal(vN, vV);

	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// D is a surface distribution function and obeys:
	// D(vH)*HdotN is normalized (over half-spere)
	// Specifically, this is the normalized phong model
	const float D = ((n+2)/(2*M_PI))*pow(HdotN, n);

	// torrance-sparrow visibility term divided by VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	
	// Schlick's approximation
	const float fFres = fresnelReflectance(VdotH, F0);

	// torrance-sparrow:
	// (F * G * D) / (4 * LdotN * VdotN)
	// Division by VdotN is done in VisibDiv()
	// and division by LdotN is removed since 
	// outgoing radiance is determined by:
	// BRDF * LdotN * L()
	float fSpec = (fFres * fVdivDots * D) / 4;
	
	// sum up: diff + spec
	float3 res = Cd * (LdotN/M_PI) + Cs * fSpec;	// LdotN canceled out in specular term
	return res;
}

// optional variant (self shadowing factor)
// vN is the shade normal (from a bump/normal map)
// vN2 represents the normalized interpolated vertex normal
float3 BRDF2_ts_nphong(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32, float F0=0.04)
{
	float3 res = BRDF_ts_nphong(vN, vL, vV, Cd, Cs, n, F0);
	return saturate(4*dot(vL, vN2)) * res;
}

// this is the Torrance-Sparrow model but using the Beckmann distribution
float3 BRDF_ts_beckmann(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22, float F0=0.04)
{
	// reflect hack when view vector is occluded
	// (doesn't seem to be needed)
	//vN = FixMyNormal(vN, vV);

	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	
	// D is a surface distribution function and obeys:
	// D(vH)*HdotN is normalized (over half-spere)
	// Specifically, this is the Beckmann surface distribution function
	// D = exp(-tan^2(\theta_h)/m^2) / (pi * m^2 * cos^4(\theta_h));
	// where \theta_h = acos(HdotN)
	const float fSqCSnh = HdotN*HdotN;
	const float fSqCSnh_m2 = fSqCSnh*m*m;
	//const float numerator = exp(-pow(tan(acos(HdotN))/m,2));
	const float numerator = exp(-((1-fSqCSnh)/max(fSqCSnh_m2, FLT_EPSILON)));		// faster than tan+acos
	const float D = numerator / (M_PI*max(fSqCSnh_m2*fSqCSnh, FLT_EPSILON));

	// torrance-sparrow visibility term divided by VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	
	// Schlick's approximation
	const float fFres = fresnelReflectance(VdotH, F0);

	// torrance-sparrow:
	// (F * G * D) / (4 * LdotN * VdotN)
	// Division by VdotN is done in VisibDiv()
	// and division by LdotN is removed since 
	// outgoing radiance is determined by:
	// BRDF * LdotN * L()
	float fSpec = (fFres * fVdivDots * D) / 4;
	
	// sum up: diff + spec
	float3 res = Cd * (LdotN/M_PI) + Cs * fSpec;	// LdotN canceled out in specular term
	return res;
}

// optional variant (self shadowing factor)
// vN is the shade normal (from a bump/normal map)
// vN2 represents the normalized interpolated vertex normal
float3 BRDF2_ts_beckmann(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22, float F0=0.04)
{
	float3 res = BRDF_ts_beckmann(vN, vL, vV, Cd, Cs, m, F0);
	return saturate(4*dot(vL, vN2)) * res;
}


float toBeckmannParam(const float n)
{
	// remap to beckmann roughness parameter by matching
	// the normalization constants in the surface distribution functions.
	float m = sqrt(2 / (n+2));
	return m;
}

float toNPhong(const float m)
{
	// remap to normalized phong roughness parameter by matching
	// the normalization constants in the surface distribution functions.
	float n = (2 / (m*m)) - 2;
	return n;
}


//-------------------------- Alternative Tilt BRDF

float CalcTiltNormalization(const float brdfAng, const int n);

// this is an alternative to the normalized Phong model used in the Torrance-Sparrow model.
// This model supports a tilted peak as opposed to the default nphong case where brdfAng=0
float3 BRDF_ts_nphong_tilt(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32, float F0=0.04)
{	
	float3 vH = normalize(vV+vL);

	// reflect hack when view vector is occluded
	// (doesn't seem to be needed)
	//vN = FixMyNormal(vN, vV);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));

	// D is a surface distribution function and obeys:
	// p(vH) = D(vH)*HdotN where p is normalized (over half-spere)
	// Specifically, this is a variant of the normalized phong model.
	// In this version p reaches its peak at the angle: brdfAng
	// if Vh is the angle between vN and vH then
	// p(vH) = fMultNorm * cos(Vh-brdfAng)^n
	// note that the integration domain remains the half-sphere
	// relative to the normal vN and that when
	// brdfAng is zero the result is identical to the regular nphong.
	
	const int ninc = n + 1;		// this increment is to match nphong which
								// has the division by n_dot_h built into it
	
	// hopefully this is resolved at compilation time
	float fMultNorm = CalcTiltNormalization(brdfAng, ninc);
	const float co2 = cos((brdfAng*M_PI)/180);
	const float si2 = sin((brdfAng*M_PI)/180);
	
	// Evaluate pdf and D
	const float co1 = dot(vH, vN);
	const float si1 = sqrt(1-co1*co1);
	const float H2dotN = saturate(co2*co1 + si2*si1);
	
	const float p = fMultNorm*pow(H2dotN, ninc);
	const float D = p / max(HdotN, FLT_EPSILON);

	// torrance-sparrow visibility term over VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	
	// Schlick's approximation
	const float fFres = fresnelReflectance(VdotH, F0);

	// torrance-sparrow
	float fSpec = (fFres * fVdivDots * D) / 4;
	
	// sum up: diff + spec
	float3 res = Cd * (LdotN/M_PI) + Cs * fSpec;	// LdotN canceled out in specular term
	return res;
}

// optional variant (self shadowing factor)
// vN is the shade normal (from a bump/normal map)
// vN2 represents the normalized interpolated vertex normal
float3 BRDF2_ts_nphong_tilt(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32, float F0=0.04)
{
	float3 res = BRDF_ts_nphong_tilt(vN, vL, vV, Cd, Cs, brdfAng, n, F0);
	return saturate(4*dot(vL, vN2)) * res;
}



////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Same BRDF functions but with Fresnel disabled /////////////////////
////////////////////////////////////////////////////////////////////////////////////////



float3 BRDF_ts_nphong_nofr(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32)
{
	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	const float D = ((n+2)/(2*M_PI))*pow(HdotN, n);
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	float fSpec = (fVdivDots * D) / 4;

	return Cd * (LdotN/M_PI) + Cs * fSpec;	// LdotN canceled out in specular term
}

float3 BRDF2_ts_nphong_nofr(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float n=32)
{
	float3 res = BRDF_ts_nphong_nofr(vN, vL, vV, Cd, Cs, n);
	return saturate(4*dot(vL, vN2)) * res;
}


float3 BRDF_ts_beckmann_nofr(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22)
{
	// microfacet normal
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	const float fSqCSnh = HdotN*HdotN;
	const float fSqCSnh_m2 = fSqCSnh*m*m;
	const float numerator = exp(-((1-fSqCSnh)/max(fSqCSnh_m2, FLT_EPSILON)));		// faster than tan+acos
	const float D = numerator / (M_PI*max(fSqCSnh_m2*fSqCSnh, FLT_EPSILON));

	// torrance-sparrow visibility term divided by VdotN
	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	float fSpec = (fVdivDots * D) / 4;
	
	return Cd * (LdotN/M_PI) + Cs * fSpec;	// LdotN canceled out in specular term
}

float3 BRDF2_ts_beckmann_nofr(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float m=0.22)
{
	float3 res = BRDF_ts_beckmann_nofr(vN, vL, vV, Cd, Cs, m);
	return saturate(4*dot(vL, vN2)) * res;
}



float3 BRDF_ts_nphong_tilt_nofr(float3 vN, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32)
{	
	float3 vH = normalize(vV+vL);

	// the various dot products
	const float LdotN = saturate(dot(vL, vN));
	const float VdotN = saturate(dot(vV, vN));
	const float VdotH = saturate(dot(vV, vH));
	const float HdotN = saturate(dot(vH, vN));
	
	// diffuse
	const int ninc = n + 1;
	
	// hopefully this is resolved at compilation time
	float fMultNorm = CalcTiltNormalization(brdfAng, ninc);
	const float co2 = cos((brdfAng*M_PI)/180);
	const float si2 = sin((brdfAng*M_PI)/180);
	
	const float co1 = dot(vH, vN);
	const float si1 = sqrt(1-co1*co1);
	const float H2dotN = saturate(co2*co1 + si2*si1);
	const float p = fMultNorm*pow(H2dotN, ninc);
	const float D = p / max(HdotN, FLT_EPSILON);

	const float fVdivDots = VisibDiv(LdotN, VdotN, VdotH, HdotN);
	float fSpec = (fVdivDots * D) / 4;
	
	return Cd * (LdotN/M_PI) + Cs * fSpec;	// LdotN canceled out in specular term
}

float3 BRDF2_ts_nphong_tilt_nofr(float3 vN, float3 vN2, float3 vL, float3 vV, float3 Cd, float3 Cs, float brdfAng=1, float n=32)
{
	float3 res = BRDF_ts_nphong_tilt_nofr(vN, vL, vV, Cd, Cs, brdfAng, n);
	return saturate(4*dot(vL, vN2)) * res;
}






////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Utility Functions ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


float CalcTiltNormalization(const float brdfAng, const int n)
{
	// if \theta_h is the angle between vN and vH then the pdf is
	// p(vH) = fMultNorm * cos(\theta_h-brdfAng)^n
	// note that when brdfAng is zero this is the regular nphong
	
	// this function computes fMultNorm as the
	// reciprocal value of p integrated over the hemisphere
	// oriented relative to vN.
	const float v0 = M_PI - ((brdfAng*M_PI)/180);
	
	// After substitution the integral can be expressed as:
	// area = 2 pi * ( sin(v0)* Int cos(u)^{n+1} du - cos(v0)* Int cos(u)^n * sin(u) du )
	// Both integrals are integrated over the domain: ]v0-pi; v0-(pi/2)[
	float fLterm = 0;
	int m = n+1;
	float factor = 1;
	
	const float t1 = v0 - (M_PI/2);
	const float t0 = v0 - M_PI;
	while(m>0)
	{	
		const float R1 = pow( cos(t1), m-1)*sin(t1);
		const float R0 = pow( cos(t0), m-1)*sin(t0);
		fLterm += (factor/m)*(R1-R0);	
		factor *= ((float) m-1) / m;
		m -= 2;
	}
	if(m==0) fLterm += factor*(t1-t0);
	
	float fRterm = (-1.0/(n+1)) * ( pow( cos(t1), n+1) - pow( cos(t0), n+1) );
	const float fNorm = (2*M_PI) * (sin(v0)*fLterm - cos(v0)*fRterm);
	
	return 1/fNorm;
}

float3 FixMyNormal(float3 vN, float3 vV)
{
	const float VdotN = dot(vV,vN);
	if(VdotN<=0)
	{
		vN = vN - 2*vV * VdotN;
	}
	return vN;
}

#endif