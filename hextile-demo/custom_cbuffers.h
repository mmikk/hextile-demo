#ifndef __CUSTOMCBUFFERS_H__
#define __CUSTOMCBUFFERS_H__

#include "shader_base.h"



#if defined(GROUND_EXAMPLE) || defined(__cplusplus)
unistruct cbMatGroundShader
{
	float g_fTileRate;
	float g_fBumpIntensity;
};
#endif

#if defined(SPHERE_EXAMPLE) || defined(__cplusplus)
unistruct cbMatSphereShader
{
	float g_fTileRate;
	float g_fBumpIntensity;
};
#endif


#if defined(ROCK_EXAMPLE) || defined(__cplusplus)
unistruct cbMatRockShader
{
	float g_fTileRate;
	float g_fBumpIntensity;
};
#endif

#if defined(PIRATE_EXAMPLE) || defined(__cplusplus)
unistruct cbMatPirateShader
{
	float g_fTileRate;
	float g_fBumpIntensity;
};
#endif




#endif