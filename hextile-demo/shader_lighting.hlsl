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



#if defined(GROUND_EXAMPLE)
float4 GroundExamplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);
	
	//float3 albedo=pow(float3(72, 72, 72)/255.0,2.2);

	float3 albedo = pow(float3(70.0,101.0,125.0)/255.0, 2.2);


	float3 vN = nrmBaseNormal;

	float2 st0 = 3*g_DetailTileRate*In.TextureUV.xy;

	if(g_bHexColorEnabled)
	{
		float4 color=0.0;
		float3 weights=0.0;
		
		if(g_useRegularTiling)
		{
			color = g_trx_d.Sample(g_samWrap, st0);
		}
		else if(g_bUseHistoPreserv)
		{
			hex2colTex_histo(color, weights, 
					  g_trx_transfer_d, g_trx_invtransfer_d, g_trx_basis_d, g_samWrap, 
					  st0, 0.0);
		}
		else
		{
			hex2colTex(color, weights, g_trx_d, g_samWrap, st0, 0.0);
		}
		albedo = color.xyz;
	}

	if(g_bHexNormalEnabled)
	{
		float4 color=0.0;
		float3 weights=0.0;
		
		if(g_useRegularTiling)
		{
			color = g_trx_n.Sample(g_samWrap, st0);
		}
		else if(g_bUseHistoPreserv)
		{
			hex2colTex_histo(color, weights, 
					  g_trx_transfer_n, g_trx_invtransfer_n, g_trx_basis_n, g_samWrap, 
					  st0, 0.0);
		}
		else
		{
			float2 dHduv;
			bumphex2derivNMap(dHduv, weights, g_trx_n, g_samWrap, st0, 0.0);
			color = float4(0.5*float3(-dHduv.xy,1.0)+0.5, 1.0);
			if(g_bFlipVertDeriv) { color.y = 1.0-color.y; }

		}

		//float3 vNt = normalize(2*color.xyz-1.0);
		float2 dHduv = TspaceNormalToDerivative(2*color.xyz-1.0);
		
		float3 surfGrad = SurfgradFromTBN(dHduv, mikktsTangent, mikktsBitangent);
	
		// resolve
		vN = ResolveNormalFromSurfaceGradient(surfGrad);
	}

	return float4(Epilogue(In, vN, albedo, 0.4),1);
}
#endif

#if defined(SPHERE_EXAMPLE)
float4 SphereExamplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);

	float3 albedo=pow(float3(106, 106, 106)/255.0,2.2);
		
	return float4(Epilogue(In, nrmBaseNormal, albedo),1);
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

	return float4(Epilogue(In, vN),1);
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
	const float lightIntensity = 1.5*M_PI;		// 2.35
	//float3 col = shadow*lightIntensity*float3(1,0.95,0.85)*BRDF2_ts_nphong(vNfinal, nrmBaseNormal, vLdir, vVdir, albedo, float3(1,1,1), spow);
	float3 col = shadow*lightIntensity*BRDF2_ts_ggx(vNfinal, nrmBaseNormal, vLdir, vVdir, albedo, float3(1,1,1), smoothness);

	// old school cheesy ambientish effect
	col += albedo*ao*IncomingEnergy(vNfinal);

	if(g_bIndirectSpecular)
	{
		float3 vR = reflect(-vVdir, vNfinal);
		float3 vRspec = GetSpecularDominantDir(vNfinal, vR, roughness);
		float VdotNsat = saturate(dot(vNfinal, vVdir));
		float2 tab_fg = g_table_FG.SampleLevel(g_samClamp, float2(VdotNsat, smoothness), 0.0);
		float specColor = 0.04;
	
		float specularOcc = ApproximateSpecularSelfOcclusion(vRspec, nrmBaseNormal);
		col += specularOcc*(tab_fg.x*specColor + tab_fg.y)*IncomingEnergy(vRspec);
	}

	
	if(g_bShowNormalsWS) col = pow(0.5*vNfinal+0.5, 2.2);

	//col = pow(0.5*(70*dPdy)+0.5, 2.2);

	return col;
}