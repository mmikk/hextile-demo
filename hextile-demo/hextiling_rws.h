#ifndef __HEXTILINGRWS_H__
#define __HEXTILINGRWS_H__

// this variant is designed for large open worlds to preserve
// fractional precision with planar and triplanar projection.
// this is done by assuming relative world space (RWS) is used to
// produce the sampling coordinate st but the offset to absolute world
// space is used to produce the (per frame) constant st_offs (scale by tile rate is applied to both)

#include "hextiling.h"



// Output:\ weights associated with each hex tile and integer centers
void TriangleGridRWS(out float w1, out float w2, out float w3, 
				  out int2 vertex1, out int2 vertex2, out int2 vertex3,
				  float2 st, float2 st_offs)
{
	// Scaling of the input
	st *= 2 * sqrt(3);
	st_offs *= 2 * sqrt(3);

	// Skew input space into simplex triangle grid
	const float2x2 gridToSkewedGrid = 
		float2x2(1.0, -0.57735027, 0.0, 1.15470054);
	float2 skewedCoord = mul(gridToSkewedGrid, st);
	float2 skewedCoord_offs = mul(gridToSkewedGrid, st_offs);

	// separate out large 2D integer offset
	int2 baseId_offs = int2( floor( skewedCoord_offs ));
	float2 comb_skew = skewedCoord + frac(skewedCoord_offs);
	int2 baseId = int2( floor( comb_skew )) + baseId_offs;
	float3 temp = float3( frac( comb_skew ), 0);
	temp.z = 1.0 - temp.x - temp.y;

	float s = step(0.0, -temp.z);
	float s2 = 2*s-1;

	w1 = -temp.z*s2;
	w2 = s - temp.y*s2;
	w3 = s - temp.x*s2;

	vertex1 = baseId + int2(s,s);
	vertex2 = baseId + int2(s,1-s);
	vertex3 = baseId + int2(1-s,s);
}

// Input:\ nmap is a normal map
// Input:\ r increase contrast when r>0.5
// Output:\ deriv is a derivative dHduv wrt units in pixels
// Output:\ weights shows the weight of each hex tile
void bumphex2derivNMapRWS(out float2 deriv, out float3 weights,
					   Texture2D nmap, SamplerState samp, float2 st, float2 st_offs,
					   float rotStrength, float r=0.5)
{
	float2 dSTdx = ddx(st), dSTdy = ddy(st);

	// Get triangle info
	float w1, w2, w3;
	int2 vertex1, vertex2, vertex3;
	TriangleGridRWS(w1, w2, w3, vertex1, vertex2, vertex3, st, st_offs);

	float2x2 rot1 = LoadRot2x2(vertex1, rotStrength);
	float2x2 rot2 = LoadRot2x2(vertex2, rotStrength);
	float2x2 rot3 = LoadRot2x2(vertex3, rotStrength);

	float2 cen1 = MakeCenST(vertex1);
	float2 cen2 = MakeCenST(vertex2);
	float2 cen3 = MakeCenST(vertex3);

	float2 st1 = mul(st, rot1) + frac(mul(st_offs - cen1, rot1) + cen1) + hash(vertex1);
	float2 st2 = mul(st, rot2) + frac(mul(st_offs - cen2, rot2) + cen2) + hash(vertex2);
	float2 st3 = mul(st, rot3) + frac(mul(st_offs - cen3, rot3) + cen3) + hash(vertex3);

	// Fetch input
	float2 d1 = sampleDeriv(nmap, samp, st1, 
							mul(dSTdx, rot1), mul(dSTdy, rot1));
	float2 d2 = sampleDeriv(nmap, samp, st2, 
							mul(dSTdx, rot2), mul(dSTdy, rot2));
	float2 d3 = sampleDeriv(nmap, samp, st3, 
							mul(dSTdx, rot3), mul(dSTdy, rot3));

	d1 = mul(rot1, d1); d2 = mul(rot2, d2); d3 = mul(rot3, d3);

	// produce sine to the angle between the conceptual normal
	// in tangent space and the Z-axis
	float3 D = float3( dot(d1,d1), dot(d2,d2), dot(d3,d3));
	float3 Dw = sqrt(D/(1.0+D));
	
	Dw = lerp(1.0, Dw, g_fallOffContrast);	// 0.6
	float3 W = Dw*pow(float3(w1, w2, w3), g_exp);	// 7
	W /= (W.x+W.y+W.z);
	if(r!=0.5) W = Gain3(W, r);

	deriv = W.x * d1 + W.y * d2 + W.z * d3;
	weights = ProduceHexWeights(W.xyz, vertex1, vertex2, vertex3);
}


// Input:\ tex is a texture with color
// Input:\ r increase contrast when r>0.5
// Output:\ color is the blended result
// Output:\ weights shows the weight of each hex tile
void hex2colTexRWS(out float4 color, out float3 weights,
				Texture2D tex, SamplerState samp, float2 st, float2 st_offs,
				float rotStrength, float r=0.5)
{
	float2 dSTdx = ddx(st), dSTdy = ddy(st);

	// Get triangle info
	float w1, w2, w3;
	int2 vertex1, vertex2, vertex3;
	TriangleGridRWS(w1, w2, w3, vertex1, vertex2, vertex3, st, st_offs);

	float2x2 rot1 = LoadRot2x2(vertex1, rotStrength);
	float2x2 rot2 = LoadRot2x2(vertex2, rotStrength);
	float2x2 rot3 = LoadRot2x2(vertex3, rotStrength);

	float2 cen1 = MakeCenST(vertex1);
	float2 cen2 = MakeCenST(vertex2);
	float2 cen3 = MakeCenST(vertex3);

	float2 st1 = mul(st, rot1) + frac(mul(st_offs - cen1, rot1) + cen1) + hash(vertex1);
	float2 st2 = mul(st, rot2) + frac(mul(st_offs - cen2, rot2) + cen2) + hash(vertex2);
	float2 st3 = mul(st, rot3) + frac(mul(st_offs - cen3, rot3) + cen3) + hash(vertex3);

	// Fetch input
	float4 c1 = tex.SampleGrad(samp, st1, 
							   mul(dSTdx, rot1), mul(dSTdy, rot1));
	float4 c2 = tex.SampleGrad(samp, st2,
							   mul(dSTdx, rot2), mul(dSTdy, rot2));
	float4 c3 = tex.SampleGrad(samp, st3, 
							   mul(dSTdx, rot3), mul(dSTdy, rot3));

	// use luminance as weight
	float3 Lw = float3(0.299, 0.587, 0.114);
	float3 Dw = float3( dot(c1,Lw), dot(c2,Lw), dot(c3,Lw));
	
	Dw = lerp(1.0, Dw, g_fallOffContrast);	// 0.6
	float3 W = Dw*pow(float3(w1, w2, w3), g_exp);	// 7
	W /= (W.x+W.y+W.z);
	if(r!=0.5) W = Gain3(W, r);

	color = W.x * c1 + W.y * c2 + W.z * c3;
	weights = ProduceHexWeights(W.xyz, vertex1, vertex2, vertex3);
}





#endif
