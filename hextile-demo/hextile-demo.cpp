// This sample was made by Morten S. Mikkelsen
// It illustrates how to do compositing of bump maps in complex scenarios using the surface gradient based framework.


#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
//#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
//#include "SDKMesh.h"
#include <d3d11_2.h>
#include "strsafe.h"
#include <stdlib.h>
#include <math.h>

#include "scenegraph.h"

#include "shadows.h"
#include "canvas.h"


#define DISABLE_QUAT

#include <geommath/geommath.h>
#include "shader.h"
#include "shaderpipeline.h"
#include "std_cbuffer.h"
#include "shaderutils.h"
#include "texture_rt.h"
#include "buffer.h"

#include <wchar.h>

CDXUTDialogResourceManager g_DialogResourceManager;
CDXUTTextHelper *		g_pTxtHelper = NULL;

CFirstPersonCamera                  g_Camera;


ID3D11Buffer * g_pGlobalsCB = NULL;

#define WIDEN2(x)		L ## x
#define WIDEN(x)		WIDEN2(x)






#include "meshimport/meshdraw.h"


#ifndef M_PI
	#define M_PI 3.1415926535897932384626433832795
#endif

bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );

void myFrustum(float * pMat, const float fLeft, const float fRight, const float fBottom, const float fTop, const float fNear, const float fFar);



static int g_iCullMethod = 1;


static int g_iMenuVisib = 1;

static bool g_bShowNormalsWS = false;
static bool g_bIndirectSpecular = true;
static bool g_bEnableShadows = true;

static bool g_bHexColEnabled = true;
static bool g_bHexNormalEnabled = false;
static bool g_bHistoPreservEnabled = false;
static bool g_bRegularTilingEnabled = false;
static int g_showWeightsMode = 0;

const float initialTileRate = 5.0f;

enum PARAM_MODE
{
	ADJUST_TILING_RATE=0,
	ADJUST_ROT_STRENGTH,
	ADJUST_COL_CONTRAST,
	ADJUST_NORMAL_CONTRAST,

	NUM_PARAMS_TO_TWEAK
};

static int g_iToggleParamWeAdjust = ADJUST_TILING_RATE;
static float g_DetailTileRate = initialTileRate;
static float g_RotStrength = 0.0f;
static float g_ColorFakeContrast = 0.7f;
static float g_NormalFakeContrast = 0.5f;

//static float frnd() { return (float) (((double) (rand() % (RAND_MAX+1))) / RAND_MAX); }

CTextureObject g_tex_depth;

CShadowMap g_shadowMap;
CCanvas g_canvas;

void InitApp()
{
    
	g_Camera.SetRotateButtons( true, false, false );
    g_Camera.SetEnablePositionMovement( true );


	const float scale = 0.04f;

    g_Camera.SetScalers( 0.2f*0.005f, 3*100.0f * scale );
    DirectX::XMVECTOR vEyePt, vEyeTo;

	Vec3 cam_pos = 75.68f*Normalize(Vec3(16,0,40));	// normal

	vEyePt = DirectX::XMVectorSet(cam_pos.x, cam_pos.y, -cam_pos.z, 1.0f);
	//vEyeTo = DirectX::XMVectorSet(0.0f, 2.0f, 0.0f, 1.0f);
	vEyeTo = DirectX::XMVectorSet(10.0f, 2.0f, 0.0f, 1.0f);


	// surfgrad demo
	const float g_O = 2*2.59f;
	const float g_S = -1.0f;		// convert unity scene to right hand frame.

	vEyeTo = DirectX::XMVectorSet(g_S*3.39f+g_O, 1.28f+0.3, -0.003f+1.5, 1.0f);
	vEyePt = DirectX::XMVectorSet(g_S*3.39f+g_O-10, 1.28f+2.5, -0.003f, 1.0f);

    g_Camera.SetViewParams( vEyePt, vEyeTo );

}

void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos( 2, 0 );
	g_pTxtHelper->SetForegroundColor(DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));
	g_pTxtHelper->DrawTextLine( DXUTGetFrameStats(true) );

	if(g_iMenuVisib!=0)
	{
		g_pTxtHelper->DrawTextLine(L"This scene illustrates practical real-time hex-tiling.\n");

		g_pTxtHelper->DrawTextLine(L"Rotate the camera by using the mouse while pressing and holding the left mouse button.\n");
		g_pTxtHelper->DrawTextLine(L"Move the camera by using the arrow keys or: w, a, s, d\n");
		g_pTxtHelper->DrawTextLine(L"Hide menu using the x key.\n");


		
		// N
		if(g_bShowNormalsWS)
			g_pTxtHelper->DrawTextLine(L"Show Normals in world space enabled (toggle using n)\n");
		else g_pTxtHelper->DrawTextLine(L"Show Normals in world space disabled (toggle using n)\n");
		/*
		// R
		if(g_bIndirectSpecular)
			g_pTxtHelper->DrawTextLine(L"Indirect Specular Reflection enabled (toggle using r)\n");
		else g_pTxtHelper->DrawTextLine(L"Indirect Specular Reflection disabled (toggle using r)\n");	
		*/
		// I
		if(g_bEnableShadows)
			g_pTxtHelper->DrawTextLine(L"Shadows enabled (toggle using i)\n");
		else g_pTxtHelper->DrawTextLine(L"Shadows disabled (toggle using i)\n");

		// V
		if(g_showWeightsMode==0)
			g_pTxtHelper->DrawTextLine(L"Hex Weights OFF (toggle using v)\n");
		else if(g_showWeightsMode==1)
			g_pTxtHelper->DrawTextLine(L"Hex Weights ON with overlay (toggle using v)\n");
		else
		{
			assert(g_showWeightsMode==2);
			g_pTxtHelper->DrawTextLine(L"Hex Weights ON (toggle using v)\n");
		}

		// C
		if(g_bHexColEnabled)
			g_pTxtHelper->DrawTextLine(L"hex colors enabled (toggle using c)\n");
		else g_pTxtHelper->DrawTextLine(L"hex colors disabled (toggle using c)\n");

		// B
		if(g_bHexNormalEnabled)
			g_pTxtHelper->DrawTextLine(L"hex normals enabled (toggle using b)\n");
		else g_pTxtHelper->DrawTextLine(L"hex normals disabled (toggle using b)\n");

		// H
		if(g_bHistoPreservEnabled)
			g_pTxtHelper->DrawTextLine(L"Histogram-Preservation enabled (toggle using h)\n");
		else g_pTxtHelper->DrawTextLine(L"Histogram-Preservation disabled (toggle using h)\n");


		// T
		if(g_bRegularTilingEnabled)
			g_pTxtHelper->DrawTextLine(L"Regular tiling enabled (toggle using t)\n");
		else g_pTxtHelper->DrawTextLine(L"Regular tiling disabled (toggle using t)\n");

		// M
		WCHAR dest_str[256];
		WCHAR tmp_str[256];
		swprintf(dest_str, L"Parameter ");
	
		if(g_iToggleParamWeAdjust == ADJUST_TILING_RATE)
			swprintf(tmp_str, L"tiling rate %2.3f", g_DetailTileRate);
		else if(g_iToggleParamWeAdjust == ADJUST_ROT_STRENGTH)
			swprintf(tmp_str, L"rotation strength %2.3f", g_RotStrength);
		else if(g_iToggleParamWeAdjust == ADJUST_COL_CONTRAST)
			swprintf(tmp_str, L"color contrast %1.3f", g_ColorFakeContrast);
		else
		{
			assert(g_iToggleParamWeAdjust==ADJUST_NORMAL_CONTRAST);
			swprintf(tmp_str, L"normal contrast %1.3f ", g_NormalFakeContrast);
		}

		wcscat(dest_str, tmp_str);
		swprintf(tmp_str, L"(toggle which parameter to adjust using m)");
		wcscat(dest_str, tmp_str);

		g_pTxtHelper->DrawTextLine(dest_str);

		g_pTxtHelper->SetForegroundColor(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
		g_pTxtHelper->DrawTextLine(L"\t\tAdjust active parameter by using the mouse while pressing and holding the middle mouse button. Reset all four using the r key.\n");
		g_pTxtHelper->DrawTextLine(L"\t\tShuffle textures for color and normal on keys o and p.\n");
	}

	g_pTxtHelper->End();
}


void render_surface(ID3D11DeviceContext* pd3dImmediateContext, bool bSimpleLayout)
{
	RenderSceneGraph(pd3dImmediateContext, bSimpleLayout, false);
}


Mat44 g_m44Proj, g_m44InvProj, g_mViewToScr, g_mScrToView;


Vec3 XMVToVec3(const DirectX::XMVECTOR vec)
{
	return Vec3(DirectX::XMVectorGetX(vec), DirectX::XMVectorGetY(vec), DirectX::XMVectorGetZ(vec));
}

Vec4 XMVToVec4(const DirectX::XMVECTOR vec)
{
	return Vec4(DirectX::XMVectorGetX(vec), DirectX::XMVectorGetY(vec), DirectX::XMVectorGetZ(vec), DirectX::XMVectorGetW(vec));
}

Mat44 ToMat44(const DirectX::XMMATRIX &dxmat)
{
	Mat44 res;

	for (int c = 0; c < 4; c++)
		SetColumn(&res, c, XMVToVec4(dxmat.r[c]));

	return res;
}


void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, 
                                  double fTime, float fElapsedTime, void* pUserContext )
{
	HRESULT hr;

	//const float fTimeDiff = DXUTGetElapsedTime();

	// clear screen
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* pDSV = g_tex_depth.GetDSV();//DXUTGetD3D11DepthStencilView();
	//DXUTGetD3D11DepthStencil();

	
	Vec3 vToPoint = XMVToVec3(g_Camera.GetLookAtPt());

	Vec3 cam_pos = XMVToVec3(g_Camera.GetEyePt());
    Mat44 world_to_view = ToMat44(g_Camera.GetViewMatrix() );	// get world to view projection

	Mat44 mZflip; LoadIdentity(&mZflip);
	SetColumn(&mZflip, 2, Vec4(0,0,-1,0));
#ifndef LEFT_HAND_COORDINATES
	world_to_view = mZflip * world_to_view * mZflip;
#else
	world_to_view = world_to_view * mZflip;
#endif
	
	Mat44 m44LocalToWorld; LoadIdentity(&m44LocalToWorld);
	Mat44 m44LocalToView = world_to_view * m44LocalToWorld;
	Mat44 Trans = g_m44Proj * world_to_view;

	
	

	// fill constant buffers
	D3D11_MAPPED_SUBRESOURCE MappedSubResource;

	// prefill shadow map
	if(g_bEnableShadows) g_shadowMap.RenderShadowMap(pd3dImmediateContext, g_pGlobalsCB, GetSunDir());

	// fill constant buffers
	const Mat44 view_to_world = ~world_to_view;
	V( pd3dImmediateContext->Map( g_pGlobalsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbGlobals *)MappedSubResource.pData)->g_mWorldToView = Transpose(world_to_view);
	((cbGlobals *)MappedSubResource.pData)->g_mViewToWorld = Transpose(view_to_world);
	((cbGlobals *)MappedSubResource.pData)->g_mScrToView = Transpose(g_mScrToView);
	((cbGlobals *)MappedSubResource.pData)->g_mProj = Transpose(g_m44Proj);
	((cbGlobals *)MappedSubResource.pData)->g_mViewProjection = Transpose(Trans);
	((cbGlobals *)MappedSubResource.pData)->g_vCamPos = view_to_world * Vec3(0,0,0);
	((cbGlobals *)MappedSubResource.pData)->g_iWidth = DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	((cbGlobals *)MappedSubResource.pData)->g_iHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	((cbGlobals *)MappedSubResource.pData)->g_bShowNormalsWS = g_bShowNormalsWS;
	((cbGlobals *)MappedSubResource.pData)->g_bIndirectSpecular = g_bIndirectSpecular;
	((cbGlobals *)MappedSubResource.pData)->g_vSunDir = GetSunDir();
	((cbGlobals *)MappedSubResource.pData)->g_bEnableShadows = g_bEnableShadows;


	((cbGlobals *)MappedSubResource.pData)->g_bHexColorEnabled = g_bHexColEnabled;
	((cbGlobals *)MappedSubResource.pData)->g_bHexNormalEnabled = g_bHexNormalEnabled;
	((cbGlobals *)MappedSubResource.pData)->g_bUseHistoPreserv = g_bHistoPreservEnabled;
	((cbGlobals *)MappedSubResource.pData)->g_useRegularTiling = g_bRegularTilingEnabled;
	((cbGlobals *)MappedSubResource.pData)->g_DetailTileRate = g_DetailTileRate;
	((cbGlobals *)MappedSubResource.pData)->g_FakeContrastColor = g_ColorFakeContrast;
	((cbGlobals *)MappedSubResource.pData)->g_FakeContrastNormal = g_NormalFakeContrast;
	((cbGlobals *)MappedSubResource.pData)->g_rotStrength = g_RotStrength;
	((cbGlobals *)MappedSubResource.pData)->g_showWeightsMode = g_showWeightsMode;
	

    pd3dImmediateContext->Unmap( g_pGlobalsCB, 0 );

	
	// prefill depth
	const bool bRenderFront = true;
	float ClearColor[4] = { 0.03f, 0.05f, 0.1f, 0.0f };

	DXUTSetupD3D11Views(pd3dImmediateContext);

	pd3dImmediateContext->OMSetRenderTargets( 0, NULL, pDSV );
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );


	pd3dImmediateContext->RSSetState( GetDefaultRasterSolidCullBack()  );
	pd3dImmediateContext->OMSetDepthStencilState( GetDefaultDepthStencilState(), 0 );

	render_surface(pd3dImmediateContext, true);

	// resolve shadow map
	if(g_bEnableShadows) g_shadowMap.ResolveToScreen(pd3dImmediateContext, g_tex_depth.GetReadOnlyDSV(), g_pGlobalsCB);

	// restore depth state
	pd3dImmediateContext->OMSetDepthStencilState( GetDefaultDepthStencilState_NoDepthWrite(), 0 );

	// switch to back-buffer
	pd3dImmediateContext->OMSetRenderTargets( 1, &pRTV, g_tex_depth.GetReadOnlyDSV() );
	pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );


	//
	g_canvas.DrawCanvas(pd3dImmediateContext, g_pGlobalsCB);

	// restore depth state
	pd3dImmediateContext->OMSetDepthStencilState( GetDefaultDepthStencilState_NoDepthWrite(), 0 );

	
	// Do tiled forward rendering
	render_surface(pd3dImmediateContext, false);



	// fire off menu text
	RenderText();
}


int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	 
    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    DXUTSetCallbackKeyboard( OnKeyboard );

    InitApp();
    DXUTInit( true, true );
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Hex-Tiling Demo." );
	int dimX = 1280, dimY = 960;
	DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, dimX, dimY);
    //DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 1024, 768);
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}



//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------


HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;


    // Setup the camera's projection parameters
	int w = pBackBufferSurfaceDesc->Width;
	int h = pBackBufferSurfaceDesc->Height;

	const float scale = 0.01f;

	//const float fFov = 30;
	const float fNear = 10 * scale;
	const float fFar = 100000 * scale;
	//const float fNear = 45;//275;
	//const float fFar = 65;//500;
	//const float fHalfWidthAtMinusNear = fNear * tanf((fFov*((float) M_PI))/360);
	//const float fHalfHeightAtMinusNear = fHalfWidthAtMinusNear * (((float) 3)/4.0);

	const float fFov = 60;
	const float fHalfHeightAtMinusNear = fNear * tanf((fFov*((float) M_PI))/360);
	const float fHalfWidthAtMinusNear = fHalfHeightAtMinusNear * (((float) w)/h);
	

	const float fS = 1.0;// 1280.0f / 960.0f;

	myFrustum(g_m44Proj.m_fMat, -fS*fHalfWidthAtMinusNear, fS*fHalfWidthAtMinusNear, -fHalfHeightAtMinusNear, fHalfHeightAtMinusNear, fNear, fFar);


	{
		float fAspectRatio = fS;
		g_Camera.SetProjParams( (fFov*M_PI)/360, fAspectRatio, fNear, fFar );
	}


	Mat44 mToScr;
	SetRow(&mToScr, 0, Vec4(0.5*w, 0,     0,  0.5*w));
	SetRow(&mToScr, 1, Vec4(0,     -0.5*h, 0,  0.5*h));
	SetRow(&mToScr, 2, Vec4(0,     0,     1,  0));
	SetRow(&mToScr, 3, Vec4(0,     0,     0,  1));

	g_mViewToScr = mToScr * g_m44Proj;
	g_mScrToView = ~g_mViewToScr;
	g_m44InvProj = ~g_m44Proj;

	



    // Set GUI size and locations
    /*g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 245, pBackBufferSurfaceDesc->Height - 520 );
    g_SampleUI.SetSize( 245, 520 );*/

	// create render targets
	const bool bEnableReadBySampling = true;
	const bool bEnableWriteTo = true;
	const bool bAllocateMipMaps = false;
	const bool bAllowStandardMipMapGeneration = false;
	const void * pInitData = NULL;

	g_tex_depth.CleanUp();

	g_tex_depth.CreateTexture(pd3dDevice,w,h, DXGI_FORMAT_R24G8_TYPELESS, bAllocateMipMaps, false, NULL,
								bEnableReadBySampling, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, bEnableWriteTo, DXGI_FORMAT_D24_UNORM_S8_UINT,
								true);




	g_shadowMap.OnResize(pd3dDevice, g_tex_depth.GetSRV());
	PassShadowResolve(g_shadowMap.GetShadowResolveSRV());

	////////////////////////////////////////////////
	V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
	//V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------

static float g_fMouseTilingValue = -1.0f+2*(1.0f-expf(log(0.5f)*(log(2*initialTileRate)/log(2.0f))));
static float g_fMouseRotStrengthValue = 0.0f;
static float g_fMouseColorContrastValue = 0.7f;
static float g_fMouseNormalContrastValue = 0.5f;


static float CheckTilingMouseValue(const float fParamMouseValueDelta)
{
	float tempTilingValue = g_fMouseTilingValue + 0.68f*fParamMouseValueDelta;
	if(tempTilingValue>0.97f) { tempTilingValue = 0.97f; }
	else if(tempTilingValue<(-1+0.00001f)) { tempTilingValue = -1+0.00001f; }
	return tempTilingValue;
}
static void UpdateTilingRate(const float mouseTilingValue)
{
	const float K = log(1-(0.5f*mouseTilingValue+0.5f)) / log(0.5f);
	g_DetailTileRate = 0.5f * powf(2.0f, K);
}

static float CheckRotStrengthMouseValue(const float fParamMouseValueDelta)
{
	float tempRotValue = g_fMouseRotStrengthValue + 0.8f*fParamMouseValueDelta;
	if(tempRotValue>0.999f) { tempRotValue = 0.999f; }
	else if(tempRotValue<0.0f) { tempRotValue = 0.0f; }
	return tempRotValue;
}

static float CheckColorContrastMouseValue(const float fParamMouseValueDelta)
{
	float tempContrastValue = g_fMouseColorContrastValue + 0.8f*fParamMouseValueDelta;
	if(tempContrastValue>0.999f) { tempContrastValue = 0.999f; }
	else if(tempContrastValue<0.0f) { tempContrastValue = 0.0f; }
	return tempContrastValue;
}

static float CheckNormalContrastMouseValue(const float fParamMouseValueDelta)
{
	float tempContrastValue = g_fMouseNormalContrastValue + 0.8f*fParamMouseValueDelta;
	if(tempContrastValue>0.999f) { tempContrastValue = 0.999f; }
	else if(tempContrastValue<0.0f) { tempContrastValue = 0.0f; }
	return tempContrastValue;
}


LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
	const int Width = DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	const int Height = DXUTGetDXGIBackBufferSurfaceDesc()->Height;

	static bool g_bDrag = false;
	static int Xbeg, Ybeg;
    switch( uMsg )
    {
        case WM_MBUTTONDOWN:
        {
            if(uMsg == WM_MBUTTONDOWN )
            {
                Xbeg = ( int )( short )LOWORD( lParam );
                Ybeg = ( int )( short )HIWORD( lParam );
				
				g_bDrag = true;
                //m_ArcBall.OnBegin( iMouseX, iMouseY );
                //SetCapture( hWnd );
            }
            return TRUE;
        }

        case WM_MOUSEMOVE:
        {
            if( g_bDrag )
            {
                int iMouseX = ( int )( short )LOWORD( lParam );
                int iMouseY = ( int )( short )HIWORD( lParam );
                
				const float paramMouseValueDelta = ((float) (iMouseX-Xbeg))/Width;

				if(g_iToggleParamWeAdjust==ADJUST_TILING_RATE)
				{
					float tempTilingValue = CheckTilingMouseValue(paramMouseValueDelta);
					UpdateTilingRate(tempTilingValue);
				}
				else if(g_iToggleParamWeAdjust==ADJUST_ROT_STRENGTH)
				{
					float tempRotStrengthValue = CheckRotStrengthMouseValue(paramMouseValueDelta);
					g_RotStrength = 25*tempRotStrengthValue;
				}
				else if(g_iToggleParamWeAdjust==ADJUST_COL_CONTRAST)
				{
					float tempColorContrastValue = CheckColorContrastMouseValue(paramMouseValueDelta);
					g_ColorFakeContrast = tempColorContrastValue;
				}
				else if(g_iToggleParamWeAdjust==ADJUST_NORMAL_CONTRAST)
				{
					float tempNormalContrastValue = CheckNormalContrastMouseValue(paramMouseValueDelta);
					g_NormalFakeContrast = tempNormalContrastValue;
				}
				

				int iTing;
				iTing = 0;
            }
            return TRUE;
        }

        case WM_MBUTTONUP:
        {
            if(uMsg == WM_MBUTTONUP )
            {
				int iMouseX = ( int )( short )LOWORD( lParam );
                int iMouseY = ( int )( short )HIWORD( lParam );
                
				const float paramMouseValueDelta = ((float) (iMouseX-Xbeg))/Width;

				if(g_iToggleParamWeAdjust==ADJUST_TILING_RATE)
				{
					g_fMouseTilingValue = CheckTilingMouseValue(paramMouseValueDelta);
					UpdateTilingRate(g_fMouseTilingValue);
				}
				else if(g_iToggleParamWeAdjust==ADJUST_ROT_STRENGTH)
				{
					g_fMouseRotStrengthValue = CheckRotStrengthMouseValue(paramMouseValueDelta);
					g_RotStrength = 25*g_fMouseRotStrengthValue;
				}
				else if(g_iToggleParamWeAdjust==ADJUST_COL_CONTRAST)
				{
					g_fMouseColorContrastValue = CheckColorContrastMouseValue(paramMouseValueDelta);
					g_ColorFakeContrast = g_fMouseColorContrastValue;
				}
				else if(g_iToggleParamWeAdjust==ADJUST_NORMAL_CONTRAST)
				{
					g_fMouseNormalContrastValue = CheckNormalContrastMouseValue(paramMouseValueDelta);
					g_NormalFakeContrast = g_fMouseNormalContrastValue;
				}

                g_bDrag = false;
            }
        }
		
		case WM_CAPTURECHANGED:
        {
			g_bDrag = false;
		}

			/*
        case WM_CAPTURECHANGED:
        {
            if( ( HWND )lParam != hWnd )
            {
                if( ( m_nRotateMask & MOUSE_LEFT_BUTTON ) ||
                    ( m_nRotateMask & MOUSE_MIDDLE_BUTTON ) ||
                    ( m_nRotateMask & MOUSE_RIGHT_BUTTON ) )
                {
                    m_ArcBall.OnEnd();
                    ReleaseCapture();
                }
            }
            return TRUE;
        }*/
    }

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );


    return 0;
}


void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	if(bKeyDown)
	{
		if(nChar=='O')
		{
			ToggleDetailTex(true);
		}
		if(nChar=='P')
		{
			ToggleDetailTex(false);
		}

		

		if (nChar == 'X')
		{
			g_iMenuVisib = 1 - g_iMenuVisib;
		}

		if (nChar == 'N')
		{
			g_bShowNormalsWS = !g_bShowNormalsWS;
		}

		if (nChar == 'R')
		{
			g_fMouseTilingValue = -1.0f+2*(1.0f-expf(log(0.5f)*(log(2*initialTileRate)/log(2.0f))));
			g_fMouseRotStrengthValue = 0.0f;
			g_fMouseColorContrastValue = 0.7f;
			g_fMouseNormalContrastValue = 0.5f;

			g_DetailTileRate = initialTileRate;
			g_RotStrength = g_fMouseRotStrengthValue;
			g_ColorFakeContrast = g_fMouseColorContrastValue;
			g_NormalFakeContrast = g_fMouseNormalContrastValue;

			//g_bIndirectSpecular = !g_bIndirectSpecular;
		}

		if (nChar == 'I')
		{
			g_bEnableShadows = !g_bEnableShadows;
		}

		if (nChar == 'C')
		{
			g_bHexColEnabled = !g_bHexColEnabled;
		}

		if (nChar == 'B')
		{
			g_bHexNormalEnabled = !g_bHexNormalEnabled;
		}

		if (nChar == 'H')
		{
			g_bHistoPreservEnabled = !g_bHistoPreservEnabled;
		}

		if (nChar == 'T')
		{
			g_bRegularTilingEnabled = !g_bRegularTilingEnabled;
		}

		if (nChar == 'M')
		{
			g_iToggleParamWeAdjust = (g_iToggleParamWeAdjust+1) % NUM_PARAMS_TO_TWEAK;
		}

		if (nChar == 'V')
		{
			g_showWeightsMode = (g_showWeightsMode+1) % 3;
		}
	}
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
		s_bFirstTime = false;
		pDeviceSettings->d3d11.AutoCreateDepthStencil = false;

		/*
		s_bFirstTime = false;
        if( ( DXUT_D3D11_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE ) )
        {
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
        }

        // Enable 4xMSAA by default
        DXGI_SAMPLE_DESC MSAA4xSampleDesc = { 4, 0 };
        pDeviceSettings->d3d11.sd.SampleDesc = MSAA4xSampleDesc;*/
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_Camera.FrameMove( fElapsedTime );
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr;

    // Get device context
    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();


	// create text helper
	V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext) );
	g_pTxtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15 );

	InitUtils(pd3dDevice);

	// set compiler flag
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif
	//dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;

	// create constant buffers
    D3D11_BUFFER_DESC bd;


	memset(&bd, 0, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = (sizeof( cbGlobals )+0xf)&(~0xf);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    V_RETURN( pd3dDevice->CreateBuffer( &bd, NULL, &g_pGlobalsCB ) );

	


	InitializeSceneGraph(pd3dDevice, pd3dImmediateContext, g_pGlobalsCB);

	// create vertex decleration
	g_shadowMap.InitShadowMap(pd3dDevice, g_pGlobalsCB, 4096, 4096);
	g_canvas.InitCanvas(pd3dDevice, g_pGlobalsCB);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	g_DialogResourceManager.OnD3D11DestroyDevice();
	SAFE_DELETE( g_pTxtHelper );

	g_tex_depth.CleanUp();

	SAFE_RELEASE( g_pGlobalsCB );

	ReleaseSceneGraph();

	g_shadowMap.CleanUp();
	g_canvas.CleanUp();

	DeinitUtils();
}


// [0;1] but right hand coordinate system
void myFrustum(float * pMat, const float fLeft, const float fRight, const float fBottom, const float fTop, const float fNear, const float fFar)
{
	// first column
	pMat[0*4 + 0] = (2 * fNear) / (fRight - fLeft); pMat[0*4 + 1] = 0; pMat[0*4 + 2] = 0; pMat[0*4 + 3] = 0;

	// second column
	pMat[1*4 + 0] = 0; pMat[1*4 + 1] = (2 * fNear) / (fTop - fBottom); pMat[1*4 + 2] = 0; pMat[1*4 + 3] = 0;

	// fourth column
	pMat[3*4 + 0] = 0; pMat[3*4 + 1] = 0; pMat[3*4 + 2] = -(fFar * fNear) / (fFar - fNear); pMat[3*4 + 3] = 0;

	// third column
	pMat[2*4 + 0] = (fRight + fLeft) / (fRight - fLeft);
	pMat[2*4 + 1] = (fTop + fBottom) / (fTop - fBottom);
	pMat[2*4 + 2] = -fFar / (fFar - fNear);
	pMat[2*4 + 3] = -1;

#ifdef LEFT_HAND_COORDINATES
	for(int r=0; r<4; r++) pMat[2*4 + r] = -pMat[2*4 + r];
#endif
}