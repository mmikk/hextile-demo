#ifndef __SURFGRADFRAMEWORK_H__
#define __SURFGRADFRAMEWORK_H__

// Smallest such that 1.0 + FLT_EPSILON != 1.0.
#ifndef FLT_EPSILON
    #define FLT_EPSILON 1.192092896e-07F        
#endif

// Cached bump globals, reusable for all UVs including procedural.
static float3 sigmaX, sigmaY, nrmBaseNormal, dPdx, dPdy;
static float  flip_sign;

// Used for vertex-level tangent space (one UV set only).
static float3 mikktsTangent, mikktsBitangent;



float3 ResolveNormalFromSurfaceGradient(float3 surfGrad)
{
	return normalize(nrmBaseNormal - surfGrad);
}

// Input: vM is tangent space normal in [-1;1].
// Output: convert vM to a derivative.
float2 TspaceNormalToDerivative(float3 vM)
{
	const float scale = 1.0/128.0;

	// Ensure vM delivers a positive third component using abs() and
	// constrain vM.z so the range of the derivative is [-128; 128].
	const float3 vMa = abs(vM);
	const float z_ma = max(vMa.z, scale*max(vMa.x, vMa.y));

	// Set to match positive vertical texture coordinate axis.
	const bool gFlipVertDeriv = true;
	const float s = gFlipVertDeriv ? -1.0 : 1.0;
	return -float2(vM.x, s*vM.y)/z_ma;
}

float3 SurfgradFromTBN(float2 deriv, float3 vT, float3 vB)
{
	return deriv.x*vT + deriv.y*vB;
}

void GenBasisTB(out float3 vT, out float3 vB, float2 texST)
{
	float2 dSTdx = ddx_fine(texST);
	float2 dSTdy = ddy_fine(texST);
	float det = dot(dSTdx, float2(dSTdy.y, -dSTdy.x));
	float sign_det = det < 0.0 ? -1.0 : 1.0;
 
	// invC0 represents (dXds, dYds), but we don't divide
	// by the determinant. Instead, we scale by the sign.
	float2 invC0 = sign_det*float2(dSTdy.y, -dSTdx.y); 
	vT = sigmaX*invC0.x + sigmaY*invC0.y;
	if (abs(det) > 0.0) vT = normalize(vT);
	vB = (sign_det*flip_sign)*cross(nrmBaseNormal, vT);
}

// Surface gradient from a known "normal" such as from an object-
// space normal map. This allows us to mix the contribution with
// others, including from tangent-space normals. The vector v
// doesn't need to be unit length so long as it establishes
// the direction. It must also be in the same space as the normal.
float3 SurfgradFromPerturbedNormal(float3 v)
{
	// If k is negative then we have an inward facing vector v,
	// so we negate the surface gradient to perturb toward v.
	float3 n = nrmBaseNormal;
	float k = dot(n, v);
	return (k*n - v)/max(FLT_EPSILON, abs(k));
}

// dim: resolution of the texture deriv was sampled from.
// isDerivScreenSpace: true if deriv is already in screen space.
float3 SurfgradScaleDependent(float2 deriv, float2 texST, uint2 dim, bool isDerivScreenSpace = false)
{
	// Convert derivative to normalized (s,t) space.
	const float2 dHdST = dim*deriv;

	// Convert derivative to screen space by applying
	// the chain rule. dHdx and dHdy correspond to
	// ddx_fine(height) and ddy_fine(height).
	float2 texDx = ddx(texST);
	float2 texDy = ddy(texST);
	float dHdx = dHdST.x*texDx.x + dHdST.y*texDx.y;
	float dHdy = dHdST.x*texDy.x + dHdST.y*texDy.y;

	if (isDerivScreenSpace)
	{
		dHdx = deriv.x;
		dHdy = deriv.y; 
	}

	// Eq.3 in "Bump Mapping Unparametrized Surfaces on the GPU".
	float3 vR1 = cross(dPdy, nrmBaseNormal);
	float3 vR2 = cross(nrmBaseNormal, dPdx);
	float det = dot(dPdx, vR1);

	const float eps = 1.192093e-15F;
	float sign_det = det < 0.0 ? -1.0 : 1.0;
	float s = sign_det/max(eps, abs(det));

	return s*(dHdx*vR1 + dHdy*vR2);
}


// Used to produce a surface gradient from the gradient of a volume
// bump function such as 3D Perlin noise. Equation 2 in [Mik10].
float3 SurfgradFromVolumeGradient(float3 grad)
{
	return grad - dot(nrmBaseNormal, grad)*nrmBaseNormal;
}


// Triplanar projection is considered a special case of volume
// bump map. Weights are obtained using DetermineTriplanarWeights()
// and derivatives using TspaceNormalToDerivative().
float3 SurfgradFromTriplanarProjection(float3 triplanarWeights, float2 deriv_xplane, float2 deriv_yplane, float2 deriv_zplane)
{
	const float w0 = triplanarWeights.x;
	const float w1 = triplanarWeights.y;
	const float w2 = triplanarWeights.z;
    
	// Assume deriv_xplane, deriv_yplane, and deriv_zplane are
	// sampled using (z,y), (x,z), and (x,y), respectively.
	// Positive scales of the lookup coordinate will work
	// as well, but for negative scales the derivative components
	// will need to be negated accordingly.
	float3 grad = float3(w2*deriv_zplane.x + w1*deriv_yplane.x,
						w2*deriv_zplane.y + w0*deriv_xplane.y,
						w0*deriv_xplane.x + w1*deriv_yplane.y);

	return SurfgradFromVolumeGradient(grad);
}

// Adapted from
// http://www.slideshare.net/icastano/cascades-demo-secrets.
float3 DetermineTriplanarWeights(float k = 3.0)
{
	float3 weights = abs(nrmBaseNormal) - 0.2;
	weights = max(0, weights);
	weights = pow(weights, k);
	weights /= (weights.x + weights.y + weights.z);
	return weights;
}


// Returns dHduv where (u,v) is in pixel units at the top MIP level.
float2 DerivFromHeightMap(Texture2D hmap, SamplerState samp, float2 texST, bool isUpscaleHQ = false)
{
	float lod = hmap.CalculateLevelOfDetail(samp, texST);
	uint2 dim; hmap.GetDimensions(dim.x, dim.y);
	float2 onePixOffs = float2(1.0/dim.x, 1.0/dim.y);
	float eoffs = exp2(lod);
	float2 actualOffs = onePixOffs*eoffs;
	float2 st_r = texST + float2(actualOffs.x, 0.0);
	float2 st_u = texST + float2(0.0, actualOffs.y);

	float Hr = hmap.Sample(samp, st_r).x;
	float Hu = hmap.Sample(samp, st_u).x;
	float Hc = hmap.Sample(samp, texST).x;
	float2 dHduv = float2(Hr - Hc, Hu - Hc)/eoffs;
	float start = 0.5, end = 0.05; // start-end fade
	float mix = saturate((lod - start)/(end - start));

	if (isUpscaleHQ && mix > 0.0)
	{
		float2 f2TexCoord = dim*texST - float2(0.5, 0.5);
		float2 f2FlTexCoord = floor(f2TexCoord);
		float2 t = saturate(f2TexCoord - f2FlTexCoord);
		float2 cenST = (f2FlTexCoord + float2(0.5, 0.5))/dim;
		float4 sampUL = hmap.Gather(samp, cenST, int2(-1,-1));
		float4 sampUR = hmap.Gather(samp, cenST, int2( 1,-1));
		float4 sampLL = hmap.Gather(samp, cenST, int2(-1, 1));
		float4 sampLR = hmap.Gather(samp, cenST, int2( 1, 1));
        
		// float4(UL.wz, UR.wz) represents first scanline and so on.
		float4x4 H = {sampUL.w, sampUL.z, sampUR.w, sampUR.z,
					sampUL.x, sampUL.y, sampUR.x, sampUR.y,
					sampLL.w, sampLL.z, sampLR.w, sampLR.z,
					sampLL.x, sampLL.y, sampLR.x, sampLR.y};

		float2 A = float2(1.0 - t.x, t.x);
		float2 B = float2(1.0 - t.y, t.y);
		float4 X = 0.25*float4(A.x, 2*A.x + A.y, A.x + 2*A.y, A.y);
		float4 Y = 0.25*float4(B.x, 2*B.x + B.y, B.x + 2*B.y, B.y);
		float4 dX = 0.5*float4(-A.x, -A.y, A.x, A.y);
		float4 dY = 0.5*float4(-B.x, -B.y, B.x, B.y);
		float2 dHduv_ups = float2(dot( Y, mul(H, dX)), dot(dY, mul(H,  X)));
		dHduv = lerp(dHduv, dHduv_ups, mix);
	}

	return dHduv;
}


// Returns dHduv where (u,v) is in pixel units at the top MIP level.
float2 DerivFromHeightMapLevel(Texture2D hmap, SamplerState samp, float2 texST, float lod_in, bool isUpscaleHQ=false)
{
	uint2 dim; hmap.GetDimensions(dim.x, dim.y);
	float2 onePixOffs = float2(1.0/dim.x, 1.0/dim.y);
	float lod = max(0.0, lod_in);
	float eoffs = exp2(lod);
	float2 actualOffs = onePixOffs*eoffs;
	float2 st_r = texST + float2(actualOffs.x, 0.0);
	float2 st_u = texST + float2(0.0, actualOffs.y);

	float Hr = hmap.SampleLevel(samp, st_r, lod).x;
	float Hu = hmap.SampleLevel(samp, st_u, lod).x;
	float Hc = hmap.SampleLevel(samp, texST, lod).x;
	float2 dHduv = float2(Hr - Hc, Hu - Hc)/eoffs;
	float start = 0.5, end = 0.05; // start-end fade
	float mix = saturate((lod - start)/(end - start));

	if (isUpscaleHQ && mix > 0.0)
	{
		float2 f2TexCoord = dim*texST - float2(0.5, 0.5);
		float2 f2FlTexCoord = floor(f2TexCoord);
		float2 t = saturate(f2TexCoord - f2FlTexCoord);
		float2 cenST = (f2FlTexCoord + float2(0.5, 0.5))/dim;
		float4 sampUL = hmap.Gather(samp, cenST, int2(-1,-1));
		float4 sampUR = hmap.Gather(samp, cenST, int2( 1,-1));
		float4 sampLL = hmap.Gather(samp, cenST, int2(-1, 1));
		float4 sampLR = hmap.Gather(samp, cenST, int2( 1, 1));
        
		// float4(UL.wz, UR.wz) represents first scanline and so on.
		float4x4 H = {sampUL.w, sampUL.z, sampUR.w, sampUR.z,
					sampUL.x, sampUL.y, sampUR.x, sampUR.y,
					sampLL.w, sampLL.z, sampLR.w, sampLR.z,
					sampLL.x, sampLL.y, sampLR.x, sampLR.y};

		float2 A = float2(1.0 - t.x, t.x);
		float2 B = float2(1.0 - t.y, t.y);
		float4 X = 0.25*float4(A.x, 2*A.x + A.y, A.x + 2*A.y, A.y);
		float4 Y = 0.25*float4(B.x, 2*B.x + B.y, B.x + 2*B.y, B.y);
		float4 dX = 0.5*float4(-A.x, -A.y, A.x, A.y);
		float4 dY = 0.5*float4(-B.x, -B.y, B.x, B.y);
		float2 dHduv_ups = float2(dot( Y, mul(H, dX)), dot(dY, mul(H,  X)));
		dHduv = lerp(dHduv, dHduv_ups, mix);
	}

	return dHduv;
}


// fix backward facing normal
float3 FixNormal(float3 N, float3 V)
{
	return dot(N,V)>=0.0 ? N : (N-2.0*dot(N,V)*V);
}


// Calculate ddx(pos) and ddy(pos) analytically.
// Practical when ddx and ddy are unavailable.
// pos: surface position of the pixel being shaded.
// norm: represents nrmBaseNormal of the position being shaded.
// mInvViewProjScr: transformation from the screen
// [0;width] x [0;height] x [0;1] to the space pos and norm are in.
// x0, y0: the current fragment coordinates (pixel center at .5).
void ScreenDerivOfPosNoDDXY(out float3 dPdx, out float3 dPdy,
							float3 pos, float3 norm, float4x4 mInvViewProjScr,
							float x0, float y0)
{    
	float4 plane = float4(norm.xyz, -dot(pos, norm));
	float4x4 mInvViewProjScrT = transpose(mInvViewProjScr);
	float4 planeScrSpace = mul(mInvViewProjScrT, plane);

	// Ax + By + Cz + D = 0 --> z = -(A/C)x - (B/C)y - D/C
	// Intersection point at (x, y, -(A/C)x - (B/C)y - D/C, 1).
	const float sign_z = planeScrSpace.z < 0.0 ? -1.0 : 1.0;
	const float nz = sign_z*max(FLT_EPSILON, abs(planeScrSpace.z));

	const float ac = -planeScrSpace.x/nz;
	const float bc = -planeScrSpace.y/nz;
	const float dc = -planeScrSpace.w/nz;

	float4 C2 = mInvViewProjScrT[2];
	float4 v0 = mInvViewProjScrT[0] + ac*C2;
	float4 v1 = mInvViewProjScrT[1] + bc*C2;
	float4 v2 = mInvViewProjScrT[3] + dc*C2;

	// 4D intersection point in world space.
	float4 ipw = v0*x0 + v1*y0 + v2;

	// Use derivative of f/g --> (f'*g - f*g')/g^2.
	float denom = max(FLT_EPSILON, ipw.w*ipw.w);
	dPdx = (v0.xyz*ipw.w - ipw.xyz*v0.w)/denom;
	dPdy = (v1.xyz*ipw.w - ipw.xyz*v1.w)/denom;

	// If mInvViewProjScr is in normalized screen space [-1;1]^2
	// then scale dPdx and dPdy by 2/width and 2/height,
	// respectively. Also, negate dPdy if there's a Y-axis flip.
	// dPdx *= 2.0/width; dPdy *= 2.0/height;
}




// dir: normalized vector in same space as surface pos and normal.
// bumpScale: p' = p + bumpScale*DisplacementMap(s,t)*normal.
float2 ProjectVecToTextureSpace(float3 dir, float2 texST, float bumpScale, bool skipProj = false)
{
	float2 texDx = ddx(texST);
	float2 texDy = ddy(texST);
	float3 vR1 = cross(dPdy, nrmBaseNormal);
	float3 vR2 = cross(nrmBaseNormal, dPdx);
	float  det = dot(dPdx, vR1);

	const float eps = 1.192093e-15F;
	float sgn = det < 0.0 ? -1.0 : 1.0;
	float s = sgn/max(eps, abs(det));

	float2 dirScr  = s*float2(dot(vR1, dir), dot(vR2, dir));
	float2 dirTex  = texDx*dirScr.x + texDy*dirScr.y;
	float  dirTexZ = dot(nrmBaseNormal, dir);

	// To search heights in [0;1] range use dirTex.xy/dirTexZ.
	s = skipProj ? 1.0 : 1.0/max(FLT_EPSILON, abs(dirTexZ));
	return s*bumpScale*dirTex;
}

// initialST: initial texture coordinate before parallax correction.
// corrOffs: parallax-corrected offset from initialST.
float3 TexSpaceOffsToSurface(float2 initialST, float2 corrOffs)
{
	float2 texDx = ddx(initialST);
	float2 texDy = ddy(initialST);
	float det = texDx.x*texDy.y - texDx.y*texDy.x;

	const float eps = 1.192093e-15F;
	float sgn = det < 0.0 ? -1.0 : 1.0;
	float s = sgn/max(eps, abs(det));

	// Transform corrOffs from texture space to screen space.
	// Use 2x2 inverse of [ddx(initialST) | ddy(initialST)]
	float vx = s*( texDy.y*corrOffs.x - texDy.x*corrOffs.y);
	float vy = s*(-texDx.y*corrOffs.x + texDx.x*corrOffs.y);

	// Transform screen-space vector to the surface.
	return vx*sigmaX + vy*sigmaY;
}



#endif
