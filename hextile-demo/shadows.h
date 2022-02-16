#ifndef __SHADOWS_H__
#define __SHADOWS_H__

#include "texture_rt.h"
#include "shader.h"
#include "shaderpipeline.h"

struct Vec3;

class CShadowMap
{
public:
	void InitShadowMap(ID3D11Device* pd3dDevice, ID3D11Buffer * pGlobalsCB, int width, int height, bool isHalfPrecision=false);
	void RenderShadowMap(ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer * pGlobalsCB, const Vec3 &sunDir);
	void ResolveToScreen(ID3D11DeviceContext* pd3dImmediateContext, ID3D11DepthStencilView * pDSV_readonly, ID3D11Buffer * pGlobalsCB);
	void OnResize(ID3D11Device* pd3dDevice, ID3D11ShaderResourceView * texDepthSRV);
	ID3D11ShaderResourceView * GetShadowResolveSRV();
	void CleanUp();
	
	CShadowMap();
	~CShadowMap();

private:

   CTextureObject m_tex_shadowmap;
   CTextureObject m_ScreenResolveRT;


	CShader m_vert_shader;
	CShader m_pix_shader;
	CShaderPipeline m_ShaderPipeline;

	ID3D11Buffer * m_pSMapCB;

	ID3D11DepthStencilState * m_pDepthStencilStateNotEqual_NoDepthWrite;

};


#endif