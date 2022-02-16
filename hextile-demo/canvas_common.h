#ifndef __CANVASCOMMON_H__
#define __CANVASCOMMON_H__

float3 GetCanvasColor(float3 dir)
{
	float intensity = 1 + 0.4*pow(smoothstep(0.7, 1.0, -dot(g_vSunDir, dir)), 1.5);

#if 1
	float3 skyColor = intensity*2*0.2*lerp(2*float3(0.15,0.1,0.2), 2.7*float3(0.5,0.55,1.0), 0.5*dir.y+0.5);
#else
	float3 shadowColor = pow(2*0.2*2*float3(0.15,0.1,0.2), 2.2);
	float3 frontColor = pow(2*0.2*2.7*float3(0.7,0.85,1.0), 2.2);

	float3 skyColor = intensity*lerp(shadowColor, frontColor, 0.5*dir.y+0.5);
#endif

	return skyColor;
}


#endif
