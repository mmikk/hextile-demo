#include "shaderutils.h"

#include "DXUT.h"
#include <d3d11_2.h> 


static ID3D11SamplerState * g_pSamplerStateWrap = NULL;
static ID3D11SamplerState * g_pSamplerStateClamp = NULL;
static ID3D11SamplerState * m_pSamplerShadowPCF = NULL;

static ID3D11RasterizerState * g_pRasterizerStateSolid = NULL;
static ID3D11RasterizerState * g_pRasterizerStateWireframe = NULL;
static ID3D11RasterizerState * g_pRasterizerStateSolidCullFront = NULL;

static ID3D11DepthStencilState * g_pDepthStencilState = NULL;
static ID3D11DepthStencilState * g_pDepthStencilState_NoDepthWrite = NULL;


bool InitUtils(ID3D11Device* pd3dDevice)
{
	HRESULT hr;

	// Create solid and wireframe rasterizer state objects
    D3D11_RASTERIZER_DESC RasterDesc;
    ZeroMemory( &RasterDesc, sizeof( D3D11_RASTERIZER_DESC ) );
    RasterDesc.FillMode = D3D11_FILL_SOLID;
    RasterDesc.CullMode = D3D11_CULL_FRONT;
    RasterDesc.DepthClipEnable = TRUE;
	V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &g_pRasterizerStateSolidCullFront ) );

	RasterDesc.CullMode = D3D11_CULL_BACK;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &g_pRasterizerStateSolid ) );

    RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    V_RETURN( pd3dDevice->CreateRasterizerState( &RasterDesc, &g_pRasterizerStateWireframe ) );

	// Create a depthstencil state
    D3D11_DEPTH_STENCIL_DESC	DSDesc;
    DSDesc.DepthEnable =        TRUE;
    DSDesc.DepthFunc =          D3D11_COMPARISON_LESS_EQUAL;
    DSDesc.DepthWriteMask =     D3D11_DEPTH_WRITE_MASK_ALL;
    DSDesc.StencilEnable =      FALSE;
    hr = pd3dDevice->CreateDepthStencilState( &DSDesc, &g_pDepthStencilState );

	DSDesc.DepthWriteMask =     D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = pd3dDevice->CreateDepthStencilState( &DSDesc, &g_pDepthStencilState_NoDepthWrite );


	// Create samplers
	D3D11_SAMPLER_DESC SSDesc;
    ZeroMemory( &SSDesc, sizeof( D3D11_SAMPLER_DESC ) );
    SSDesc.Filter =         D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SSDesc.AddressU =       D3D11_TEXTURE_ADDRESS_WRAP;
    SSDesc.AddressV =       D3D11_TEXTURE_ADDRESS_WRAP;
    SSDesc.AddressW =       D3D11_TEXTURE_ADDRESS_WRAP;
    SSDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SSDesc.MaxAnisotropy =  16;
    SSDesc.MinLOD =         0;
    SSDesc.MaxLOD =         D3D11_FLOAT32_MAX;
    V_RETURN( pd3dDevice->CreateSamplerState( &SSDesc, &g_pSamplerStateWrap) );

	SSDesc.AddressU =       D3D11_TEXTURE_ADDRESS_CLAMP;
    SSDesc.AddressV =       D3D11_TEXTURE_ADDRESS_CLAMP;
    SSDesc.AddressW =       D3D11_TEXTURE_ADDRESS_CLAMP;
	V_RETURN( pd3dDevice->CreateSamplerState( &SSDesc, &g_pSamplerStateClamp) );

	ZeroMemory( &SSDesc, sizeof( D3D11_SAMPLER_DESC ) );
	SSDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	SSDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	SSDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	SSDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	SSDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

	for(int c=0; c<4; c++) SSDesc.BorderColor[0]=1.0f;

	V_RETURN( pd3dDevice->CreateSamplerState( &SSDesc, &m_pSamplerShadowPCF ) );
        


	return true;
}

void DeinitUtils()
{
	SAFE_RELEASE( g_pDepthStencilState );
	SAFE_RELEASE( g_pDepthStencilState_NoDepthWrite );
	SAFE_RELEASE( g_pRasterizerStateSolid );
    SAFE_RELEASE( g_pRasterizerStateWireframe );
	SAFE_RELEASE( g_pRasterizerStateSolidCullFront );
    SAFE_RELEASE( g_pSamplerStateWrap );
	SAFE_RELEASE( g_pSamplerStateClamp );
	SAFE_RELEASE( m_pSamplerShadowPCF );
}


ID3D11SamplerState * GetDefaultShadowSampler()
{
	return m_pSamplerShadowPCF;
}

ID3D11SamplerState * GetDefaultSamplerWrap()
{
	return g_pSamplerStateWrap;
}

ID3D11SamplerState * GetDefaultSamplerClamp()
{
	return g_pSamplerStateClamp;
}

ID3D11RasterizerState * GetDefaultRasterSolid()
{
	return g_pRasterizerStateSolid;
}

ID3D11RasterizerState * GetDefaultRasterWire()
{
	return g_pRasterizerStateWireframe;
}

ID3D11RasterizerState * GetDefaultRasterSolidCullBack()
{
	return GetDefaultRasterSolid();
}

ID3D11RasterizerState * GetDefaultRasterSolidCullFront()
{
	return g_pRasterizerStateSolidCullFront;
}

ID3D11DepthStencilState * GetDefaultDepthStencilState()
{
	return g_pDepthStencilState;
}

ID3D11DepthStencilState * GetDefaultDepthStencilState_NoDepthWrite()
{
   return g_pDepthStencilState_NoDepthWrite;
}

unsigned int GetDXGIBitWidth( DXGI_FORMAT fmt )
{
	switch( fmt )
	{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 64;

		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			return 32;

		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
			return 16;
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_A8_UNORM:
			return 8;
		case DXGI_FORMAT_R1_UNORM:
			return 1;
		default:
			return 0;
	}
}