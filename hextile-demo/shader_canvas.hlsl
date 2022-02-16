
//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
#include "std_cbuffer.h"
#include "canvas_common.h"


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


VS_OUTPUT DrawCanvasVS( VS_INPUT input)
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


//Texture2D g_depthMap;

SamplerState g_samWrap;
SamplerState g_samClamp;
SamplerComparisonState g_samShadow;


float4 DrawCanvasPS( VS_OUTPUT In ) : SV_TARGET0
{
	//uint2 coord = uint2(In.Position.x, In.Position.y);
	//float depth = g_depthMap[coord].x;
	float depth = 1.0;

	// position in camera space
	float4 v4ScrPos = float4(In.Position.xy, depth, 1);
	float4 v4ViewPos = mul(v4ScrPos, g_mScrToView);
	float3 surfPosInView = v4ViewPos.xyz / v4ViewPos.w;

	// actual world space position
	float3 dir = normalize( mul(surfPosInView, (float3x3) g_mViewToWorld).xyz );

	return float4( GetCanvasColor(dir), 1.0);
}
