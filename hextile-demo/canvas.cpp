#include "canvas.h"
#include "shaderutils.h"


void CCanvas::InitCanvas(ID3D11Device* pd3dDevice, ID3D11Buffer * pGlobalsCB)
{
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
	m_vert_shader.CompileShaderFunction(pd3dDevice, L"shader_canvas.hlsl", pDefines, "DrawCanvasVS", "vs_5_0", g_dwShaderFlags );
	m_pix_shader.CompileShaderFunction(pd3dDevice, L"shader_canvas.hlsl", pDefines, "DrawCanvasPS", "ps_5_0", g_dwShaderFlags );


	m_ShaderPipeline.SetVertexShader(&m_vert_shader);
	m_ShaderPipeline.SetPixelShader(&m_pix_shader);

	//m_ShaderPipeline.RegisterConstBuffer("cbShadowMap", m_pSMapCB);
	m_ShaderPipeline.RegisterConstBuffer("cbGlobals", pGlobalsCB);

	//m_ShaderPipelines.RegisterResourceView("g_shadowMap", m_tex_shadowmap.GetSRV());
	
	// register samplers
	m_ShaderPipeline.RegisterSampler("g_samWrap", GetDefaultSamplerWrap() );
	m_ShaderPipeline.RegisterSampler("g_samClamp", GetDefaultSamplerClamp() );
	m_ShaderPipeline.RegisterSampler("g_samShadow", GetDefaultShadowSampler() );


	D3D11_DEPTH_STENCIL_DESC	DSDesc;
    DSDesc.DepthEnable =        TRUE;
    DSDesc.DepthFunc =          D3D11_COMPARISON_EQUAL;
    DSDesc.DepthWriteMask =     D3D11_DEPTH_WRITE_MASK_ALL;
    DSDesc.StencilEnable =      FALSE;
	DSDesc.DepthWriteMask =     D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = pd3dDevice->CreateDepthStencilState( &DSDesc, &m_pDepthStencilStateEqual_NoDepthWrite );
}

void CCanvas::DrawCanvas(ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer * pGlobalsCB)
{
	pd3dImmediateContext->OMSetDepthStencilState( m_pDepthStencilStateEqual_NoDepthWrite, 0 );
												   
	// switch to back-buffer
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

void CCanvas::CleanUp()
{
	m_vert_shader.CleanUp();
	m_pix_shader.CleanUp();

	SAFE_RELEASE( m_pDepthStencilStateEqual_NoDepthWrite );
}
	
CCanvas::CCanvas()
{

}

CCanvas::~CCanvas()
{

}