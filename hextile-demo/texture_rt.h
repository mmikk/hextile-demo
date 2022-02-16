#ifndef __TEXTURERT_H__
#define __TEXTURERT_H__

//#include <d3dx11.h>
#include <d3d11_2.h> 

class CTextureObject
{
public:
	ID3D11RenderTargetView * GetRTV(const int iMipLevel = 0);
	ID3D11ShaderResourceView * GetSRV();
	ID3D11DepthStencilView * GetDSV();
	ID3D11DepthStencilView * GetReadOnlyDSV();
	ID3D11Texture2D * GetTexResource();
	bool CreateTexture(ID3D11Device* pd3dDevice, const int w, const int h, DXGI_FORMAT tex_format,
					   const bool bAllocateMipMaps, const bool bAllowStandardMipMapGeneration, const void * pInitData, 
					   const bool bEnableReadBySampling, DXGI_FORMAT rd_format, const bool bEnableWriteTo, DXGI_FORMAT wr_format,
					   const bool bThisIsADepthBuffer=false);

	void CleanUp();

	bool DumpTexture(ID3D11DeviceContext* pd3dImmediateContext, void * dataPtr[], int widthOut[], int heightOut[], int pixSize[]);

	CTextureObject();
	~CTextureObject();
private:
	int m_iNrRenderTargets;		// same as the amount of mip maps
	ID3D11Texture2D * m_mem;
	ID3D11RenderTargetView ** m_pRTs;
	ID3D11ShaderResourceView * m_srv;
	ID3D11DepthStencilView * m_dsv, * m_dsv_readonly;
};

#endif