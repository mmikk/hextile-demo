#include "std_cbuffer.h"
#include "custom_cbuffers.h"
#include "illum.h"
#include "surfgrad_framework.h"
#include "hextiling.h"
#include "noise.h"
#include "canvas_common.h"


//-----------------------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------------------

Texture2D g_norm_tex;
Texture2D g_albedo_tex;
Texture2D g_smoothness_tex;
Texture2D g_ao_tex;
Texture2D g_mask_tex;

Texture2D g_trx_d;
Texture2D g_trx_transfer_d;
Texture2D g_trx_invtransfer_d;
Texture2D g_trx_basis_d;

Texture2D g_trx_n;
Texture2D g_trx_transfer_n;
Texture2D g_trx_invtransfer_n;
Texture2D g_trx_basis_n;


Texture2D g_shadowResolve;
Texture2D g_table_FG;

SamplerState g_samWrap;
SamplerState g_samClamp;
SamplerComparisonState g_samShadow;

//--------------------------------------------------------------------------------------
// shader input/output structure
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Position     : POSITION;
    float3 Normal       : NORMAL;
    float2 TextureUV    : TEXCOORD0;
	float2 TextureUV2   : TEXCOORD1;
    float4 Tangent		: TEXCOORD2;
    //float4 BiTangent    : TEXCOORD2;
};

struct VS_OUTPUT
{
    float4 Position     : SV_POSITION;
    float4 Diffuse      : COLOR0;
    float2 TextureUV    : TEXCOORD0;
	float2 TextureUV2   : TEXCOORD1;
	float3 normal			: TEXCOORD2;
    float4 tangent			: TEXCOORD3;
};


VS_OUTPUT RenderSceneVS( VS_INPUT input)
{
	VS_OUTPUT Output;
	float3 vNormalWorldSpace;

	float3 vP = mul( float4(input.Position.xyz,1.0), g_mLocToWorld ).xyz;
	
	// Transform the position from object space to homogeneous projection space
	Output.Position = mul( float4(vP,1.0), g_mViewProjection );



	// position & normal
	Output.normal = normalize(mul((float3x3) g_mWorldToLocal, input.Normal.xyz));	// inverse transposed for normal
	Output.tangent = float4( normalize(mul(input.Tangent.xyz, (float3x3) g_mLocToWorld)), input.Tangent.w );

	Output.TextureUV = input.TextureUV.xy;
	Output.TextureUV2 = input.TextureUV2.xy;
	
	// flip to upper left origin
	Output.TextureUV = float2(Output.TextureUV.x,1-Output.TextureUV.y); 
	Output.TextureUV2 = float2(Output.TextureUV2.x,1-Output.TextureUV2.y); 
	Output.tangent.w = -Output.tangent.w;

	return Output;    
}


// this function should return true when we observe
// the back-face of a two-sided material
bool IsFlipNormal()
{
	return false;
}

float GetTileRate()
{
	return 0.05*g_DetailTileRate;
}

static bool g_bFlipVertDeriv = true;
static float3 surfPosInWorld;
static float3 surfPosInView;

void Prologue(VS_OUTPUT In)
{
	// position in camera space
	float4 v4ScrPos = float4(In.Position.xyz, 1);
	float4 v4ViewPos = mul(v4ScrPos, g_mScrToView);
	surfPosInView = v4ViewPos.xyz / v4ViewPos.w;

	// actual world space position
	surfPosInWorld = mul(float4(surfPosInView.xyz,1.0), g_mViewToWorld).xyz;

	// relative world space
	float3 relSurfPos = mul(surfPosInView, (float3x3) g_mViewToWorld);

	// mikkts for conventional vertex-level tangent space
	// (no normalization is mandatory). Using "bitangent on the fly"
	// option in xnormal to reduce vertex shader outputs.
	float sign_w = In.tangent.w > 0.0 ? 1.0 : -1.0;
	mikktsTangent = In.tangent.xyz;
	mikktsBitangent = sign_w*cross(In.normal.xyz, In.tangent.xyz);

	// Prepare for surfgrad formulation w/o breaking mikkTSpace
	// compliance (use same scale as interpolated vertex normal).
	float renormFactor = 1.0/length(In.normal.xyz);
	mikktsTangent   *= renormFactor;
	mikktsBitangent *= renormFactor;
	nrmBaseNormal    = renormFactor*In.normal.xyz;

	// Handle two-sided materials. Surface gradients, tangent, and
    // bitangent don't flip as a result of flipping the base normal.
	if ( IsFlipNormal() ) nrmBaseNormal = -nrmBaseNormal;
	
	// The variables below (plus nrmBaseNormal) need to be
	// recomputed in the case of back-to-back bump mapping.

#if 1
		// NO TRANSLATION! Just 3x3 transform to avoid precision issues.
		dPdx = ddx_fine(relSurfPos);
		dPdy = ddy_fine(relSurfPos);
#else
		// use this path if ddx and ddy are not available options such as during deferred or RTX
		float3 V_c = normalize(-surfPosInView);
		float3 dPdx_c, dPdy_c;
		float3 nrmBaseNormal_c = mul((float3x3) g_mViewToWorld, nrmBaseNormal);	// inverse transposed for normal
		nrmBaseNormal_c = FixNormal(nrmBaseNormal_c, V_c);
		ScreenDerivOfPosNoDDXY(dPdx_c, dPdy_c, surfPosInView, nrmBaseNormal_c, transpose(g_mScrToView), In.Position.x, In.Position.y);
		dPdx = mul(dPdx_c, (float3x3) g_mViewToWorld);		// instead of using g_mScrToView use g_mScrToRelativeWorldSpace
		dPdy = mul(dPdy_c, (float3x3) g_mViewToWorld);		// to skip the additional transformations between world and view space.
#endif

	sigmaX = dPdx - dot(dPdx, nrmBaseNormal)*nrmBaseNormal;
	sigmaY = dPdy - dot(dPdy, nrmBaseNormal)*nrmBaseNormal;
	flip_sign = dot(dPdy, cross(nrmBaseNormal, dPdx)) < 0 ? -1 : 1;

}

float3 Epilogue(VS_OUTPUT In, float3 vN, float3 albedo=pow(float3(72, 72, 72)/255.0,2.2), float smoothness=0.5, float ao=1.0);


float3 FetchSignedVector(Texture2D tex, SamplerState samp, float2 st)
{
	return 2*tex.Sample( samp, st ).xyz-1;
}

float3 FetchSignedVectorLevel(Texture2D tex, SamplerState samp, float2 st, float lod)
{
	return 2*tex.SampleLevel( samp, st, lod ).xyz-1;
}

float3 FetchSignedVectorGrad(Texture2D tex, SamplerState samp, float2 st, float2 dSTdx, float2 dSTdy)
{
	return 2*tex.SampleGrad( samp, st, dSTdx, dSTdy ).xyz-1;
}

float3 FetchSignedVectorFromCubeLevel(TextureCube tex, SamplerState samp, float3 Dir, float lod)
{
	return 2*tex.SampleLevel( samp, Dir, lod ).xyz-1;
}

float3 FetchSignedVectorFromCubeGrad(TextureCube tex, SamplerState samp, float3 Dir, float3 dDirdx, float3 dDirdy)
{
	return 2*tex.SampleGrad( samp, Dir, dDirdx, dDirdy ).xyz-1;
}

float2 FetchDeriv(Texture2D tex, SamplerState samp, float2 st)
{
	float3 vec = FetchSignedVector(tex, samp, st);
	return TspaceNormalToDerivative(vec);
}

float2 FetchDerivLevel(Texture2D tex, SamplerState samp, float2 st, float lod)
{
	float3 vec = FetchSignedVectorLevel(tex, samp, st, lod);
	return TspaceNormalToDerivative(vec);
}

float2 FetchDerivGrad(Texture2D tex, SamplerState samp, float2 st, float2 dSTdx, float2 dSTdy)
{
	float3 vec = FetchSignedVectorGrad(tex, samp, st, dSTdx, dSTdy);
	return TspaceNormalToDerivative(vec);
}

static float3 g_sphereAlbedo = pow(float3(77, 45, 32)/255.0,2.2);
//static  float3 g_sphereAlbedo = 1.3*pow(float3(77, 55, 48)/255.0,2.2);


void FixupPrologueNoDDXY(VS_OUTPUT In)
{
	surfPosInView = mul(float4(surfPosInWorld.xyz,1.0), g_mWorldToView).xyz;
	float3 V_c = normalize(-surfPosInView);
	float3 dPdx_c, dPdy_c;
	float3 nrmBaseNormal_c = mul((float3x3) g_mViewToWorld, nrmBaseNormal);	// inverse transposed for normal
	nrmBaseNormal_c = FixNormal(nrmBaseNormal_c, V_c);
	ScreenDerivOfPosNoDDXY(dPdx_c, dPdy_c, surfPosInView, nrmBaseNormal_c, transpose(g_mScrToView), In.Position.x, In.Position.y);
	dPdx = mul(dPdx_c, (float3x3) g_mViewToWorld);		// instead of using g_mScrToView use g_mScrToRelativeWorldSpace
	dPdy = mul(dPdy_c, (float3x3) g_mViewToWorld);		// to skip the additional transformations between world and view space.

	// 
	sigmaX = dPdx - dot(dPdx, nrmBaseNormal)*nrmBaseNormal;
	sigmaY = dPdy - dot(dPdy, nrmBaseNormal)*nrmBaseNormal;
	flip_sign = dot(dPdy, cross(nrmBaseNormal, dPdx))<0 ? -1 : 1;
}


void hex2colTex_histo(out float4 color, out float3 weights, 
					  Texture2D trx_transf, Texture2D trx_invtransf, Texture2D trx_basis, SamplerState samp, 
					  float2 st, float rotStrength)
{
	float2 dSTdx = ddx(st), dSTdy = ddy(st);

	// Get triangle info
	float w1, w2, w3;
	int2 vertex1, vertex2, vertex3;
	TriangleGrid(w1, w2, w3, vertex1, vertex2, vertex3, st);

	float2x2 rot1 = LoadRot2x2(vertex1, rotStrength);
	float2x2 rot2 = LoadRot2x2(vertex2, rotStrength);
	float2x2 rot3 = LoadRot2x2(vertex3, rotStrength);

	float2 cen1 = MakeCenST(vertex1);
	float2 cen2 = MakeCenST(vertex2);
	float2 cen3 = MakeCenST(vertex3);

	float2 st1 = mul(st - cen1, rot1) + cen1 + hash(vertex1);
	float2 st2 = mul(st - cen2, rot2) + cen2 + hash(vertex2);
	float2 st3 = mul(st - cen3, rot3) + cen3 + hash(vertex3);


	// Fetch input
	float3 G1 = trx_transf.SampleGrad(samp, st1, 
							   mul(dSTdx, rot1), mul(dSTdy, rot1));
	float3 G2 = trx_transf.SampleGrad(samp, st2,
							   mul(dSTdx, rot2), mul(dSTdy, rot2));
	float3 G3 = trx_transf.SampleGrad(samp, st3, 
							   mul(dSTdx, rot3), mul(dSTdy, rot3));

	float3 W = float3(w1, w2, w3);
	W = pow(W, g_exp);	// 7
	W /= (W.x+W.y+W.z);


	// Variance - preserving blending
	float2 vari = trx_basis.Gather(g_samClamp, float2(0.0,0.0), int2(7,1)).xy;
	float3 G = W.x * G1 + W.y * G2 + W.z * G3;
	G = G - 0.5;
	G = G * rsqrt( dot(W,W) );
	G.x *= vari.x; G.z *= vari.y;
	G = G + 0.5;

	// Fetch LUT
	float lod = trx_transf.CalculateLevelOfDetail(g_samWrap, st);
	uint2 dim; trx_invtransf.GetDimensions(dim.x, dim.y);
	float lodTcoordinate = (lod+0.5)/dim.y;

	float4 colH;
	colH.x = trx_invtransf.SampleLevel(g_samClamp, float2(G.x, lodTcoordinate), 0.0).x;
	colH.y = trx_invtransf.SampleLevel(g_samClamp, float2(G.y, lodTcoordinate), 0.0).y;
	colH.z = trx_invtransf.SampleLevel(g_samClamp, float2(G.z, lodTcoordinate), 0.0).z;
	colH.w = 1.0;
	
	float4 row0 = trx_basis.Gather(g_samClamp, float2(0.0,0.0), int2(1,1));
	float4 row1 = trx_basis.Gather(g_samClamp, float2(0.0,0.0), int2(3,1));
	float4 row2 = trx_basis.Gather(g_samClamp, float2(0.0,0.0), int2(5,1));


	color = float4( dot(row0, colH), dot(row1, colH), dot(row2, colH), 1.0);

	weights = ProduceHexWeights(W, vertex1, vertex2, vertex3);
}

void FetchColorAndWeight(out float3 color, out float3 weights, float2 st)
{
	float4 col4;
	if(g_useRegularTiling)
	{
		col4 = g_trx_d.Sample(g_samWrap, st);
		weights = 1.0;
	}
	else if(g_bUseHistoPreserv)
	{
		hex2colTex_histo(col4, weights, 
					g_trx_transfer_d, g_trx_invtransfer_d, g_trx_basis_d, g_samWrap, 
					st, g_rotStrength);
	}
	else
	{
		hex2colTex(col4, weights, g_trx_d, g_samWrap, st, g_rotStrength, g_FakeContrastColor);
	}
	color = col4.xyz;
}

void FetchDerivAndWeight(out float2 dHduv, out float3 weights, float2 st)
{
	if(g_useRegularTiling)
	{
		float4 color = g_trx_n.Sample(g_samWrap, st);
		dHduv = TspaceNormalToDerivative(2*color.xyz-1.0);
	}
	else if(g_bUseHistoPreserv)
	{
		float4 color;
		hex2colTex_histo(color, weights, 
					g_trx_transfer_n, g_trx_invtransfer_n, g_trx_basis_n, g_samWrap, 
					st, g_rotStrength);
		dHduv = TspaceNormalToDerivative(2*color.xyz-1.0);
	}
	else
	{
		bumphex2derivNMap(dHduv, weights, g_trx_n, g_samWrap, st, g_rotStrength, g_FakeContrastNormal);
	}
}


#if defined(GROUND_EXAMPLE)
float4 GroundExamplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);
	
	//float3 albedo=pow(float3(72, 72, 72)/255.0,2.2);

	float3 albedo = pow(float3(70.0,101.0,125.0)/255.0, 2.2);

	float3 sp = GetTileRate() * surfPosInWorld;

	float2 st0 = float2(sp.x, -sp.z);	// since looking at -Z in a right hand coordinate frame.

	// need to negate .y of derivative due to upper-left corner being the texture origin
	st0 = float2(st0.x, 1.0-st0.y);


	float3 vN = nrmBaseNormal;

	if(g_bHexColorEnabled)
	{
		float3 color, weights;
		FetchColorAndWeight(color, weights, st0);

		albedo = color;

		if(g_showWeightsMode==1)
			albedo *= (0.75*weights + 0.25);
		else if(g_showWeightsMode==2)
			albedo = weights;
	}

	if(g_bHexNormalEnabled)
	{
		float2 dHduv=0.0;
		float3 weights=0.0;
		FetchDerivAndWeight(dHduv, weights, st0);

		if(!g_bHexColorEnabled)
		{
			if(g_showWeightsMode==1)
				albedo *= (0.75*weights + 0.25);
			else if(g_showWeightsMode==2)
				albedo = weights;
		}

		// switch to lower-left origin
		dHduv.y *= -1.0;

		// negate back since we sampled using (x,-z)
		dHduv.y *= -1.0;

		float3 volGrad = float3(dHduv.x, 0.0, dHduv.y);
		float3 surfGrad = SurfgradFromVolumeGradient(volGrad);
		float weightY = DetermineTriplanarWeights(1.0).y;

		surfGrad *= (weightY * g_fBumpIntensity);
	
		// resolve
		vN = ResolveNormalFromSurfaceGradient(surfGrad);
	}

	return float4(Epilogue(In, vN, albedo),1);
}
#endif

void CommonTriplanarNormal(out float3 normO, out float3 weightsO, float3 position, float3 Nbase, float bumpScale)
{
	// backup base normal and patch it
	const float3 recordBaseNorm = nrmBaseNormal;
	nrmBaseNormal = Nbase;


	float3 pos = GetTileRate() * position;

	float2 sp_x = float2(-pos.z, pos.y);
	float2 sp_y = float2(pos.x, -pos.z);
	float2 sp_z = float2(pos.x, pos.y);

	// need to negate .y of derivative due to upper-left corner being the texture origin
	float2 dHduv_x=0.0, dHduv_y=0.0, dHduv_z=0.0;
	float3 weights_x=0.0, weights_y=0.0, weights_z=0.0;

	FetchDerivAndWeight(dHduv_x, weights_x, float2(sp_x.x, 1.0-sp_x.y));
	FetchDerivAndWeight(dHduv_y, weights_y, float2(sp_y.x, 1.0-sp_y.y));
	FetchDerivAndWeight(dHduv_z, weights_z, float2(sp_z.x, 1.0-sp_z.y));

	// switch to lower-left origin
	dHduv_x.y *= -1.0; dHduv_y.y *= -1.0; dHduv_z.y *= -1.0;

	// need to negate these back since we used (-z,y) and (x,-z) for sampling
	dHduv_x.x *= -1.0; dHduv_y.y *= -1.0;


	float3 weights = DetermineTriplanarWeights(3.0);
	float3 surfGrad = bumpScale * SurfgradFromTriplanarProjection(weights, dHduv_x, dHduv_y, dHduv_z);


	normO = ResolveNormalFromSurfaceGradient(surfGrad);
	weightsO = weights.x*weights_x + weights.y*weights_y + weights.z*weights_z;

	// restore base normal 
	nrmBaseNormal = recordBaseNorm;
}

void CommonTriplanarColor(out float3 colorO, out float3 weightsO, float3 position, float3 Nbase)
{
	// backup base normal and patch it
	const float3 recordBaseNorm = nrmBaseNormal;
	nrmBaseNormal = Nbase;


	float3 pos = GetTileRate() * position;

	float2 sp_x = float2(-pos.z, pos.y);
	float2 sp_y = float2(pos.x, -pos.z);
	float2 sp_z = float2(pos.x, pos.y);

	// need to negate .y of derivative due to upper-left corner being the texture origin
	float3 col_x=0.0, col_y=0.0, col_z=0.0;
	float3 weights_x=0.0, weights_y=0.0, weights_z=0.0;

	FetchColorAndWeight(col_x, weights_x, float2(sp_x.x, 1.0-sp_x.y));
	FetchColorAndWeight(col_y, weights_y, float2(sp_y.x, 1.0-sp_y.y));
	FetchColorAndWeight(col_z, weights_z, float2(sp_z.x, 1.0-sp_z.y));
	
	float3 weights = DetermineTriplanarWeights(3.0);

	colorO = weights.x*col_x + weights.y*col_y + weights.z*col_z;
	weightsO = weights.x*weights_x + weights.y*weights_y + weights.z*weights_z;

	// restore base normal 
	nrmBaseNormal = recordBaseNorm;
}

void FetchColorNormalTriPlanar(inout float3 albedo, inout float3 vN, float3 position, float3 Nbase, float bumpScale=1.0)
{
	if(g_bHexColorEnabled)
	{
		float3 color, weights;
		CommonTriplanarColor(color, weights, position, Nbase);

		albedo = color;

		if(g_showWeightsMode==1)
			albedo *= (0.75*weights + 0.25);
		else if(g_showWeightsMode==2)
			albedo = weights;
	}

	if(g_bHexNormalEnabled)
	{
		float3 weights=0.0;
		CommonTriplanarNormal(vN, weights, position, Nbase, bumpScale);

		if(!g_bHexColorEnabled)
		{
			if(g_showWeightsMode==1)
				albedo *= (0.75*weights + 0.25);
			else if(g_showWeightsMode==2)
				albedo = weights;
		}
	}
}

#if defined(SPHERE_EXAMPLE)
float4 SphereExamplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);

	float3 albedo=pow(float3(106, 106, 106)/255.0,2.2);
	float3 vN = nrmBaseNormal;

	//float3 albedo = pow(float3(70.0,101.0,125.0)/255.0, 2.2);
	FetchColorNormalTriPlanar(albedo, vN, surfPosInWorld, nrmBaseNormal);
	

	return float4(Epilogue(In, vN, albedo),1);
}
#endif


#if defined(PIRATE_EXAMPLE)
float4 PirateExamplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);
	float2 dHduv = FetchDeriv(g_norm_tex, g_samWrap, g_fTileRate*In.TextureUV.xy);
	float3 tang=mikktsTangent, bitang=mikktsBitangent;
	
	float3 surfGrad = g_fBumpIntensity * SurfgradFromTBN(dHduv, tang, bitang);
	
	// resolve
	float3 vN = ResolveNormalFromSurfaceGradient(surfGrad);

	float3 albedo = g_albedo_tex.Sample(g_samClamp, In.TextureUV.xy);

	float mask_r = g_mask_tex.Sample(g_samClamp, In.TextureUV.xy).x;
	float3 albedo2 = albedo;
	FetchColorNormalTriPlanar(albedo2, vN, surfPosInWorld, vN, mask_r);
	albedo = lerp(albedo, albedo2, mask_r);

	float smoothness = g_smoothness_tex.Sample(g_samClamp, In.TextureUV.xy).x;
	float ao = g_ao_tex.Sample(g_samClamp, In.TextureUV.xy).x;

	return float4(Epilogue(In, vN, albedo, smoothness, ao),1);
}
#endif


#if defined(ROCK_EXAMPLE)
float4 RockExamplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);
	float2 dHduv = FetchDeriv(g_norm_tex, g_samWrap, g_fTileRate*In.TextureUV.xy);
	float3 tang=mikktsTangent, bitang=mikktsBitangent;
	
	float3 surfGrad = g_fBumpIntensity * SurfgradFromTBN(dHduv, tang, bitang);
	
	// resolve
	float3 vN = ResolveNormalFromSurfaceGradient(surfGrad);
	float3 albedo=pow(float3(72, 72, 72)/255.0,2.2);

	FetchColorNormalTriPlanar(albedo, vN, surfPosInWorld, vN);

	return float4(Epilogue(In, vN, albedo),1);
}
#endif


float3 IncomingEnergy(float3 dir)
{
	return GetCanvasColor(dir);
}

// frostbite presentation (moving frostbite to pbr)
float3 GetSpecularDominantDir(float3 vN, float3 vR, float fRealRoughness)
{
    float fInvRealRough = saturate(1 - fRealRoughness);
    float lerpFactor = fInvRealRough * (sqrt(fInvRealRough)+fRealRoughness);

    return lerp(vN, vR, lerpFactor);
}

// marmoset horizon occlusion http://marmosetco.tumblr.com/post/81245981087
float ApproximateSpecularSelfOcclusion(float3 vR, float3 baseNormal)
{
    const float fFadeParam = 1.3;
    float rimmask = clamp( 1 + fFadeParam * dot(vR, baseNormal), 0.0, 1.0);
    rimmask *= rimmask;
    
    return rimmask;
}

float3 Epilogue(VS_OUTPUT In, float3 vN, float3 albedo, float smoothness, float ao)
{
	// find our tile from the pixel coordinate
	uint2 uCoord = (uint2) In.Position.xy;
	
	float roughness = (1-smoothness)*(1-smoothness);
	const float eps = 1.192093e-15F;
	//float spow = -2.0 + 2.0/max(eps,roughness*roughness);		// specular power

	float3 vNfinal = vN;
	
	// do some basic lighting
	float shadow = g_bEnableShadows ? g_shadowResolve[uCoord].x : 1.0;
	float3 vVdir = normalize( mul(-surfPosInView, (float3x3) g_mViewToWorld) );
	float3 vLdir = -g_vSunDir;			  // 31, -30
	const float lightIntensity = 2.5*M_PI;		// 2.35
	//float3 col = shadow*lightIntensity*float3(1,0.95,0.85)*BRDF2_ts_nphong(vNfinal, nrmBaseNormal, vLdir, vVdir, albedo, float3(1,1,1), spow);
	float3 col = shadow*lightIntensity*BRDF2_ts_ggx(vNfinal, nrmBaseNormal, vLdir, vVdir, albedo, float3(1,1,1), smoothness);

	// old school cheesy ambientish effect
	float mult = 0.2;
	col += mult*albedo*ao*IncomingEnergy(vNfinal);

	if(g_bIndirectSpecular)
	{
		float3 vR = reflect(-vVdir, vNfinal);
		float3 vRspec = GetSpecularDominantDir(vNfinal, vR, roughness);
		float VdotNsat = saturate(dot(vNfinal, vVdir));
		float2 tab_fg = g_table_FG.SampleLevel(g_samClamp, float2(VdotNsat, smoothness), 0.0);
		float specColor = 0.04;
	
		float specularOcc = ApproximateSpecularSelfOcclusion(vRspec, nrmBaseNormal);
		col += mult*specularOcc*(tab_fg.x*specColor + tab_fg.y)*IncomingEnergy(vRspec);
	}

	col *= 0.5;

	if(g_bShowNormalsWS) col = pow(0.5*vNfinal+0.5, 2.2);

	//col = pow(0.5*(70*dPdy)+0.5, 2.2);

	return col;
}