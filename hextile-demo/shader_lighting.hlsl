#include "std_cbuffer.h"
#include "custom_cbuffers.h"
#include "illum.h"
#include "surfgrad_framework.h"
#include "noise.h"
#include "canvas_common.h"


//-----------------------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------------------

Texture2D g_norm_tex;
Texture2D g_albedo_tex;
Texture2D g_smoothness_tex;
Texture2D g_ao_tex;

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

float4 SuperSimplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);

	float3 albedo=pow(float3(106, 106, 106)/255.0,2.2);
		
	return float4(Epilogue(In, nrmBaseNormal, albedo),1);
}

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

#if defined(BASIC_SAMPLE)
float4 BasicSamplePS( VS_OUTPUT In ) : SV_TARGET0
{
	Prologue(In);
	float2 dHduv = FetchDeriv(g_norm_tex, g_samWrap, g_fTileRate*In.TextureUV.xy);
	float3 tang=mikktsTangent, bitang=mikktsBitangent;
	
	float3 surfGrad = g_fBumpIntensity * SurfgradFromTBN(dHduv, tang, bitang);
	float3 vN = ResolveNormalFromSurfaceGradient(surfGrad);
	
	float3 albedo = g_sphereAlbedo;
	return float4(Epilogue(In, vN, albedo),1);
}
#endif


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
	const float lightIntensity = 2.5 * M_PI;		// 2.35
	//float3 col = shadow*lightIntensity*float3(1,0.95,0.85)*BRDF2_ts_nphong(vNfinal, nrmBaseNormal, vLdir, vVdir, albedo, float3(1,1,1), spow);
	float3 col = shadow*lightIntensity*float3(1,0.95,0.85)*BRDF2_ts_ggx(vNfinal, nrmBaseNormal, vLdir, vVdir, albedo, float3(1,1,1), smoothness);

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