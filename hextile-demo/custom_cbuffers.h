#ifndef __CUSTOMCBUFFERS_H__
#define __CUSTOMCBUFFERS_H__

#include "shader_base.h"


#if defined(BASIC_SAMPLE) || defined(__cplusplus)
unistruct cbMatBasicShader
{
	float g_fTileRate;
	float g_fBumpIntensity;
};
#endif

#if defined(ROCK_EXAMPLE) || defined(__cplusplus)
unistruct cbMatRockShader
{
	int g_iPad0;
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