#include "texture_rt.h"
#include "shaderutils.h"
#include "DXUT.h"
#include <assert.h>

bool CTextureObject::CreateTexture(ID3D11Device* pd3dDevice, const int w, const int h, DXGI_FORMAT tex_format,
								   const bool bAllocateMipMaps, const bool bAllowStandardMipMapGeneration, const void * pInitData, 
								   const bool bEnableReadBySampling, DXGI_FORMAT rd_format, const bool bEnableWriteTo, DXGI_FORMAT wr_format,
								   const bool bThisIsADepthBuffer)
{
	CleanUp();

	// setup structure for initial texel values if any exist
	unsigned int fmt_size = GetDXGIBitWidth(tex_format);

	D3D11_SUBRESOURCE_DATA sData;
	sData.pSysMem = pInitData;
	sData.SysMemPitch = fmt_size*w;
	sData.SysMemSlicePitch = 0;

	D3D11_SUBRESOURCE_DATA * pData = pInitData==NULL ? NULL : &sData;
		 
	// create the texture resource (allocates the gpu memory)
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = bAllocateMipMaps ? 0 : 1;		// 0 means reserve room for all right?
	desc.ArraySize = 1;
	desc.Format = tex_format;
	desc.SampleDesc.Count=1; desc.SampleDesc.Quality=0;
    desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.BindFlags = ((bEnableWriteTo && !bThisIsADepthBuffer) ? D3D11_BIND_RENDER_TARGET : 0) |
					 (bEnableReadBySampling ? D3D11_BIND_SHADER_RESOURCE : 0) |
					 ((bEnableWriteTo && bThisIsADepthBuffer) ? D3D11_BIND_DEPTH_STENCIL : 0);
	desc.MiscFlags = bAllowStandardMipMapGeneration ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;	// use ::GenerateMips()

	pd3dDevice->CreateTexture2D( &desc, pData, &m_mem );

	// create shader resource view
	if(bEnableReadBySampling)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
		ZeroMemory( &sr_desc, sizeof(sr_desc) );
		sr_desc.Format = rd_format;
		sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		sr_desc.Texture2D.MipLevels = -1;	// use all levels available
		pd3dDevice->CreateShaderResourceView(m_mem, &sr_desc, &m_srv );
	}

	// create render target(s)
	if(bEnableWriteTo)
	{
		if(!bThisIsADepthBuffer)
		{
			D3D11_TEXTURE2D_DESC cDesc;
			m_mem->GetDesc(&cDesc);
			int num_mip_maps = cDesc.MipLevels;
			
			m_pRTs = new ID3D11RenderTargetView *[num_mip_maps];
			if(m_pRTs!=NULL)
			{
				for(int m=0; m<num_mip_maps; m++)
				{
					D3D11_RENDER_TARGET_VIEW_DESC rt_desc;
					ZeroMemory( &rt_desc, sizeof(rt_desc) );
					rt_desc.Format = wr_format;
					rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					rt_desc.Texture2D.MipSlice = m;
					pd3dDevice->CreateRenderTargetView(m_mem, &rt_desc, &m_pRTs[m] );
				}
				m_iNrRenderTargets = num_mip_maps;
			}
		}
		else
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC cDDesc;
			ZeroMemory( &cDDesc, sizeof(cDDesc) );
			cDDesc.Format = wr_format;
			cDDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			cDDesc.Texture2D.MipSlice = 0;
			cDDesc.Flags = 
			pd3dDevice->CreateDepthStencilView( m_mem, &cDDesc, &m_dsv ); 

			ZeroMemory( &cDDesc, sizeof(cDDesc) );
			cDDesc.Format = wr_format;
			cDDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			cDDesc.Texture2D.MipSlice = 0;
			cDDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
			pd3dDevice->CreateDepthStencilView( m_mem, &cDDesc, &m_dsv_readonly ); 
		}
	}

	return true;
}

ID3D11RenderTargetView * CTextureObject::GetRTV(const int iMipLevel)
{
	assert(iMipLevel>=0 && iMipLevel<m_iNrRenderTargets);
	return m_pRTs[iMipLevel];
}

ID3D11ShaderResourceView * CTextureObject::GetSRV()
{
	return m_srv;
}

ID3D11DepthStencilView * CTextureObject::GetDSV()
{
	return m_dsv;
}

ID3D11DepthStencilView * CTextureObject::GetReadOnlyDSV()
{
	return m_dsv_readonly;
}

ID3D11Texture2D * CTextureObject::GetTexResource()
{
	return m_mem;
}

void CTextureObject::CleanUp()
{
	if(m_mem!=NULL) SAFE_RELEASE(m_mem);
	for(int m=0; m<m_iNrRenderTargets; m++)
		SAFE_RELEASE(m_pRTs[m]);
	if(m_pRTs!=NULL) { delete [] m_pRTs; m_pRTs=NULL; }
	if(m_srv!=NULL) SAFE_RELEASE(m_srv);
	if(m_dsv!=NULL) SAFE_RELEASE(m_dsv);
	if(m_dsv_readonly!=NULL) SAFE_RELEASE(m_dsv_readonly);
	m_iNrRenderTargets = 0;
}

CTextureObject::CTextureObject()
{
	m_iNrRenderTargets = 0;		// same as the amount of mip maps
	m_mem = NULL;
	m_pRTs = NULL;
	m_srv = NULL;
	m_dsv = NULL;
	m_dsv_readonly = NULL;
}

CTextureObject::~CTextureObject()
{
}

bool CTextureObject::DumpTexture(ID3D11DeviceContext* pd3dImmediateContext, void * dataPtr[], int widthOut[], int heightOut[], int pixSize[])
{
	D3D11_TEXTURE2D_DESC desc;
	GetTexResource()->GetDesc(&desc);

	DXGI_FORMAT tex_format = desc.Format;
	unsigned int fmt_size = (GetDXGIBitWidth(tex_format) + 7)/8;
	const int width = desc.Width;
	const int height = desc.Height;


	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;//D3D11_BIND_SHADER_RESOURCE;

	ID3D11Device* pd3dDevice = NULL; 
	pd3dImmediateContext->GetDevice(&pd3dDevice);

	ID3D11Texture2D * Staging = nullptr;
	pd3dDevice->CreateTexture2D(&desc, NULL, &Staging);

	pd3dImmediateContext->CopyResource(Staging, GetTexResource());

	void * pixPtr = new unsigned char[fmt_size * width * height];

	D3D11_MAPPED_SUBRESOURCE ResourceDesc = {};
	pd3dImmediateContext->Map(Staging, 0, D3D11_MAP_READ, 0, & ResourceDesc);

	if (ResourceDesc.pData)
	{
		for (int i = 0; i < height; ++ i)
		{
			std::memcpy((byte *) pixPtr + width * fmt_size * i, (byte *) ResourceDesc.pData + ResourceDesc.RowPitch * i, width * fmt_size);
		}
	}

	pd3dImmediateContext->Unmap(Staging, 0);

	if(Staging!=NULL) SAFE_RELEASE(Staging);

	dataPtr[0] = pixPtr;
	widthOut[0] = width;
	heightOut[0] = height;
	pixSize[0] = fmt_size;

	return true;
}