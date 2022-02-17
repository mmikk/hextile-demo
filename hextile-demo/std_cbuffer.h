#ifndef __STDCBUFFER_H__
#define __STDCBUFFER_H__

#include "shader_base.h"


unistruct cbMeshInstance
{
	Mat44 g_mLocToWorld;
	Mat44 g_mWorldToLocal;
};

unistruct cbGlobals
{
	Vec3	g_vCamPos;
	int		g_iPadQ;

	Mat44	g_mProj;
	Mat44	g_mViewProjection;
	Mat44	g_mWorldToView;
	Mat44	g_mViewToWorld;
	Mat44	g_mScrToView;

	int		g_iWidth;
	int		g_iHeight;
	int		g_bShowNormalsWS;
	int		g_bEnableShadows;

	Vec3	g_vSunDir;
	int		g_bIndirectSpecular;

	int		g_bHexColorEnabled;
	int		g_bHexNormalEnabled;
	int		g_bUseHistoPreserv;
	int		g_useRegularTiling;

	float	g_DetailTileRate;
	float	g_FakeContrast;

};

#endif