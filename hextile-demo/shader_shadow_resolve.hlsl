
//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
#include "std_cbuffer.h"
#include "shadows_cbuffer.h"


//--------------------------------------------------------------------------------------
// shader input/output structure
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	uint vert_id : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 Position     : SV_POSITION;
};


VS_OUTPUT RenderSceneVS( VS_INPUT input)
{
	VS_OUTPUT Output;

	float4 pos;
	{
		if(input.vert_id==0)
			pos = float4(-1.0,1.0, 1.0, 1.0);
		else if(input.vert_id==1)
			pos = float4(3.0,1.0, 1.0, 1.0);
		else if(input.vert_id==2)
			pos = float4(-1.0,-3.0, 1.0, 1.0);
	}

	Output.Position = pos;

	return Output;    
}


Texture2D g_depthMap;
Texture2D g_shadowMap;
SamplerComparisonState g_samShadow;


float4 ShadowResolvePS( VS_OUTPUT In ) : SV_TARGET0
{
	uint2 coord = uint2(In.Position.x, In.Position.y);
	float depth = g_depthMap[coord].x;

	// position in camera space
	float4 v4ScrPos = float4(In.Position.xy, depth, 1);
	float4 v4ViewPos = mul(v4ScrPos, g_mScrToView);
	float3 surfPosInView = v4ViewPos.xyz / v4ViewPos.w;

	// actual world space position
	float3 surfPosInWorld = mul(float4(surfPosInView.xyz,1.0), g_mViewToWorld).xyz;

	// shadow map space
	float3 smp = mul(float4(surfPosInWorld.xyz,1.0), g_mWorldToShadowMap).xyz;
#if 1
	float res = g_shadowMap.SampleCmpLevelZero(g_samShadow, smp.xy, smp.z-0.001).x;
#else
	float2 jitter[25] = {
		float2(0.563585, 0.001251), float2(0.808740, 0.193304), float2(0.479873, 0.585009), float2(0.895962, 0.350291), float2(0.746605, 0.822840), float2(0.858943, 0.174108), float2(0.513535, 0.710501), float2(0.014985, 0.303995), float2(0.364452, 0.091403), float2(0.165899, 0.147313), float2(0.445692, 0.988525), float2(0.004669, 0.119083), float2(0.377880, 0.008911), float2(0.571184, 0.531663), float2(0.607166, 0.601764), float2(0.663045, 0.166234), float2(0.352123, 0.450789), float2(0.607685, 0.057039), float2(0.802606, 0.783319), float2(0.301950, 0.519883), float2(0.726676, 0.875973), float2(0.925718, 0.955901), float2(0.142338, 0.539354), float2(0.235328, 0.462081), float2(0.209601, 0.862239) };

	uint x0 = coord.x%5;
	uint y0 = coord.y%5;

	float res = 0.0;
	for(int j=0; j<5; j++)
	{
		int j0 = j+y0; if(j0>=5) j0 -= 5;
		for(int i=0; i<5; i++)
		{
			int i0 = i+x0; if(i0>=5) i0 -= 5;

			float2 jit = jitter[j0*5+i0];
			float2 texSToffs = float2((i-2) + jit.x, (j-2) + jit.y);
			float2 texST = smp.xy + 0.8*(texSToffs/1024.0);

			res += g_shadowMap.SampleCmpLevelZero(g_samShadow, texST.xy, smp.z-0.001).x;
		}
	}
	res /= 25;
#endif

	return float4(res.xxx,1);
}
