#include "shadows.h"
#include <geommath/geommath.h>
#include "shadows_cbuffer.h"
#include "scenegraph.h"
#include <math.h>
#include "DXUT.h"

#include "shaderpipeline.h"
#include "std_cbuffer.h"
#include "shaderutils.h"


static void BuildBasis(Vec3 * vXout, Vec3 * vYout, const Vec3 vDir)
{
	Vec3 vX, vY;

	const float avx = fabsf(vDir.x);
	const float avy = fabsf(vDir.y);
	const float avz = fabsf(vDir.z);

    if(avx<=avy && avx<=avz)
    {
		vY.x=0.0f; vY.y=vDir.z; vY.z=-vDir.y;
    }
    else if(avy<=avz)
    {	
		vY.x=vDir.z; vY.y=0.0f; vY.z=-vDir.x;  
    }
    else
    {
		vY.x=vDir.y; vY.y=-vDir.x; vY.z=0.0f;  
    }

    const float fRecLen = 1.0f / sqrtf(vY.x*vY.x+vY.y*vY.y+vY.z*vY.z);
	vY.x *= fRecLen; vY.y *= fRecLen; vY.z *= fRecLen;

	vX.x = vY.y*vDir.z - vY.z*vDir.y;
	vX.y = vY.z*vDir.x - vY.x*vDir.z;
	vX.z = vY.x*vDir.y - vY.y*vDir.x;

	// return result
	*vXout = vX; *vYout = vY;
}



void CShadowMap::RenderShadowMap(ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer * pGlobalsCB, const Vec3 &sunDir)
{
	// build frame of reference for the sun
	const Vec3 vZ = -sunDir;
	Vec3 vX, vY;
	BuildBasis(&vX, &vY, vZ);
	Mat33 rotToLgt;
	SetRow(&rotToLgt, 0, vX); SetRow(&rotToLgt, 1, vY); SetRow(&rotToLgt, 2, vZ);


	// build up a bound around casters in light space
	const int nrShadowCasters = GetNumberOfShadowCastingMeshInstances();

	Vec3 vGlbMi, vGlbMa;

	for(int i=0; i<nrShadowCasters; i++)
	{
		Vec3 vMi, vMa;
		Mat44 mat;
		GetAABBoxAndTransformOfShadowCastingMeshInstance(&vMi, &vMa, &mat, i);

		for(int j=0; j<8; j++)
		{
			const Vec3 vPloc = Vec3((j&0x1)!=0 ? vMa.x : vMi.x, (j&0x2)!=0 ? vMa.y : vMi.y, (j&0x4)!=0 ? vMa.z : vMi.z);

			const Vec4 v4Pw = mat*vPloc;
			const Vec3 vP = rotToLgt * Vec3(v4Pw.x, v4Pw.y, v4Pw.z);

			if(i==0 && j==0) { vGlbMi=vP; vGlbMa=vP; }
			else
			{
				if(vGlbMi.x>vP.x) vGlbMi.x=vP.x;
				else if(vGlbMa.x<vP.x) vGlbMa.x=vP.x;
				if(vGlbMi.y>vP.y) vGlbMi.y=vP.y;
				else if(vGlbMa.y<vP.y) vGlbMa.y=vP.y;
				if(vGlbMi.z>vP.z) vGlbMi.z=vP.z;
				else if(vGlbMa.z<vP.z) vGlbMa.z=vP.z;
			}
		}
	}

	const Vec3 vCen = 0.5f*(vGlbMa+vGlbMi);

	Mat44 toLgt;
	SetRow(&toLgt, 0, Vec4(vX.x, vX.y, vX.z, -vCen.x));
	SetRow(&toLgt, 1, Vec4(vY.x, vY.y, vY.z, -vCen.y));
	SetRow(&toLgt, 2, Vec4(vZ.x, vZ.y, vZ.z, -vCen.z));
	SetRow(&toLgt, 3, Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	Vec3 vHalfSize = 0.5f*(vGlbMa-vGlbMi);

	// build ortho matrix
	Mat44 proj;
	SetRow(&proj, 0, Vec4(1.0f/vHalfSize.x, 0.0f, 0.0f, 0.0f) );
	SetRow(&proj, 1, Vec4(0.0f, 1.0f/vHalfSize.y, 0.0f, 0.0f) );
	SetRow(&proj, 2, Vec4(0.0f, 0.0f, -0.5f/vHalfSize.z, 0.5f) );
	SetRow(&proj, 3, Vec4(0.0f, 0.0f, 0.0f, 1.0f) );
			 

	Mat44 viewProj = proj * toLgt;

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedSubResource;
	V( pd3dImmediateContext->Map( pGlobalsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbGlobals *)MappedSubResource.pData)->g_mWorldToView = Transpose(toLgt);
	((cbGlobals *)MappedSubResource.pData)->g_mViewToWorld = Transpose(~toLgt);
	//((cbGlobals *)MappedSubResource.pData)->g_mScrToView = Transpose(g_mScrToView);
	((cbGlobals *)MappedSubResource.pData)->g_mProj = Transpose(proj);
	((cbGlobals *)MappedSubResource.pData)->g_mViewProjection = Transpose(viewProj);
	((cbGlobals *)MappedSubResource.pData)->g_vCamPos = (~toLgt) * Vec3(0,0,0);
	((cbGlobals *)MappedSubResource.pData)->g_iWidth = 4096;
	((cbGlobals *)MappedSubResource.pData)->g_iHeight = 4096;
	((cbGlobals *)MappedSubResource.pData)->g_iMode = 0;
	((cbGlobals *)MappedSubResource.pData)->g_bShowNormalsWS = false;
	((cbGlobals *)MappedSubResource.pData)->g_vSunDir = sunDir;
    pd3dImmediateContext->Unmap( pGlobalsCB, 0 );



	// prefill depth
	const bool bRenderFront = true;
	float ClearColor[4] = { 0.03f, 0.05f, 0.1f, 0.0f };

	// Setup the viewport to match the backbuffer
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)4096;
    vp.Height = (FLOAT)4096;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pd3dImmediateContext->RSSetViewports( 1, &vp );


	ID3D11DepthStencilView* pDSV = m_tex_shadowmap.GetDSV();
	pd3dImmediateContext->OMSetRenderTargets( 0, NULL, pDSV );
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );


	pd3dImmediateContext->RSSetState( GetDefaultRasterSolidCullBack()  );
	pd3dImmediateContext->OMSetDepthStencilState( GetDefaultDepthStencilState(), 0 );

	RenderSceneGraph(pd3dImmediateContext, true, true);

	pd3dImmediateContext->OMSetDepthStencilState( GetDefaultDepthStencilState_NoDepthWrite(), 0 );


	
	Mat44 mToScr;
	SetRow(&mToScr, 0, Vec4(0.5, 0,     0,  0.5));
	SetRow(&mToScr, 1, Vec4(0,     -0.5, 0,  0.5));
	SetRow(&mToScr, 2, Vec4(0,     0,     1,  0));
	SetRow(&mToScr, 3, Vec4(0,     0,     0,  1));

	Mat44 mWorldToSmap = mToScr * viewProj;

	V( pd3dImmediateContext->Map( m_pSMapCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbShadowMap *)MappedSubResource.pData)->g_mWorldToShadowMap = Transpose(mWorldToSmap);
    pd3dImmediateContext->Unmap( m_pSMapCB, 0 );


}

void CShadowMap::ResolveToScreen(ID3D11DeviceContext* pd3dImmediateContext, ID3D11DepthStencilView * pDSV_readonly, ID3D11Buffer * pGlobalsCB)
{
	//pd3dImmediateContext->OMSetDepthStencilState( GetDefaultDepthStencilState_NoDepthWrite(), 0 );
	pd3dImmediateContext->OMSetDepthStencilState( m_pDepthStencilStateNotEqual_NoDepthWrite, 0 );
												   
	// switch to back-buffer
	float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ID3D11RenderTargetView * pRTV = m_ScreenResolveRT.GetRTV();
	pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV, pDSV_readonly );
	pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );

	CShaderPipeline &pipe = m_ShaderPipeline;

	pipe.PrepPipelineForRendering(pd3dImmediateContext);
	

	// set streams and layout
	pd3dImmediateContext->IASetVertexBuffers( 0, 0, NULL, NULL, NULL );
	pd3dImmediateContext->IASetIndexBuffer( NULL, DXGI_FORMAT_UNKNOWN, 0 );
	pd3dImmediateContext->IASetInputLayout( NULL );

	// Set primitive topology
	pd3dImmediateContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	pd3dImmediateContext->Draw( 3, 1);
	pipe.FlushResources(pd3dImmediateContext);
}

void CShadowMap::OnResize(ID3D11Device* pd3dDevice, ID3D11ShaderResourceView * texDepthSRV)
{
	const int width = DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	const int height = DXUTGetDXGIBackBufferSurfaceDesc()->Height;

	const bool bEnableReadBySampling = true;
	const bool bEnableWriteTo = true;
	const bool bAllocateMipMaps = false;
	const bool bAllowStandardMipMapGeneration = false;
	
	m_ShaderPipeline.RegisterResourceView("g_depthMap", texDepthSRV);

	m_ScreenResolveRT.CleanUp();
	m_ScreenResolveRT.CreateTexture(pd3dDevice,width,height, DXGI_FORMAT_R8_TYPELESS, bAllocateMipMaps, false, NULL,
								  bEnableReadBySampling, DXGI_FORMAT_R8_UNORM, bEnableWriteTo, DXGI_FORMAT_R8_UNORM);
}


void CShadowMap::InitShadowMap(ID3D11Device* pd3dDevice, ID3D11Buffer * pGlobalsCB, int width, int height, bool isHalfPrecision_in)
{
	bool isHalfPrecision = false;
	const bool bEnableReadBySampling = true;
	const bool bEnableWriteTo = true;
	const bool bAllocateMipMaps = false;
	const bool bAllowStandardMipMapGeneration = false;
	DXGI_FORMAT fmt = isHalfPrecision ? DXGI_FORMAT_R16_TYPELESS : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	m_tex_shadowmap.CreateTexture(pd3dDevice,width,height, DXGI_FORMAT_R24G8_TYPELESS, bAllocateMipMaps, false, NULL,
								  bEnableReadBySampling, fmt, bEnableWriteTo, DXGI_FORMAT_D24_UNORM_S8_UINT, true);


	DWORD g_dwShaderFlags = D3D10_SHADER_OPTIMIZATION_LEVEL1;//D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    g_dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

	HRESULT hr;

	CONST D3D10_SHADER_MACRO* pDefines = NULL;
	m_vert_shader.CompileShaderFunction(pd3dDevice, L"shader_shadow_resolve.hlsl", pDefines, "RenderSceneVS", "vs_5_0", g_dwShaderFlags );
	m_pix_shader.CompileShaderFunction(pd3dDevice, L"shader_shadow_resolve.hlsl", pDefines, "ShadowResolvePS", "ps_5_0", g_dwShaderFlags );

	D3D11_BUFFER_DESC bd;

	memset(&bd, 0, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = (sizeof(cbShadowMap)+0xf)&(~0xf);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	V( pd3dDevice->CreateBuffer( &bd, NULL, &m_pSMapCB ) );


	m_ShaderPipeline.SetVertexShader(&m_vert_shader);
	m_ShaderPipeline.SetPixelShader(&m_pix_shader);

	m_ShaderPipeline.RegisterConstBuffer("cbShadowMap", m_pSMapCB);
	m_ShaderPipeline.RegisterConstBuffer("cbGlobals", pGlobalsCB);

	m_ShaderPipeline.RegisterResourceView("g_shadowMap", m_tex_shadowmap.GetSRV());
	
	// register samplers
	m_ShaderPipeline.RegisterSampler("g_samWrap", GetDefaultSamplerWrap() );
	m_ShaderPipeline.RegisterSampler("g_samClamp", GetDefaultSamplerClamp() );
	m_ShaderPipeline.RegisterSampler("g_samShadow", GetDefaultShadowSampler() );


	D3D11_DEPTH_STENCIL_DESC	DSDesc;
    DSDesc.DepthEnable =        TRUE;
    DSDesc.DepthFunc =          D3D11_COMPARISON_NOT_EQUAL;
    DSDesc.DepthWriteMask =     D3D11_DEPTH_WRITE_MASK_ALL;
    DSDesc.StencilEnable =      FALSE;
	DSDesc.DepthWriteMask =     D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = pd3dDevice->CreateDepthStencilState( &DSDesc, &m_pDepthStencilStateNotEqual_NoDepthWrite );

}

ID3D11ShaderResourceView * CShadowMap::GetShadowResolveSRV()
{
	return m_ScreenResolveRT.GetSRV();
}

void CShadowMap::CleanUp()
{
	m_tex_shadowmap.CleanUp();
	m_ScreenResolveRT.CleanUp();


	m_vert_shader.CleanUp();
	m_pix_shader.CleanUp();

	SAFE_RELEASE( m_pSMapCB );
					 
	SAFE_RELEASE( m_pDepthStencilStateNotEqual_NoDepthWrite );
}
	
CShadowMap::CShadowMap()
{
	
}

CShadowMap::~CShadowMap()
{

}