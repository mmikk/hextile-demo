#include "scenegraph.h"
#include "meshimport/meshdraw.h"

#include "shader.h"
#include "shaderpipeline.h"
#include "shaderutils.h"
#include "std_cbuffer.h"
#include "custom_cbuffers.h"
#include "buffer.h"


#include <d3d11_2.h>

#include "DXUT.h"
#include "SDKmisc.h"


#ifndef M_PI
	#define M_PI	3.1415926535897932384626433832795f
#endif

static Vec3 g_vSunDir;


enum TEX_ID
{
	PIRATE_ALBEDO=0,
	PIRATE_OCCLUSION,
	PIRATE_SMOOTHNESS,
	PIRATE_NORMALS_TS,
	PIRATE_MASK,

	ROCK_BASE_TS,

	TRX_JAGUAR,
	TRX_GRASS_SHORT_D=TRX_JAGUAR+4,
	TRX_MOSSGROUND_D=TRX_GRASS_SHORT_D+4,
	TRX_NATURE_PEBBLES_D=TRX_MOSSGROUND_D+4,
		
	TRX_PEBBLES_BEACH_CROP_N=TRX_NATURE_PEBBLES_D+4,
	TRX_SEAN_MICRO_N=TRX_PEBBLES_BEACH_CROP_N+4,
	TRX_SNOW_MELT_02A_N=TRX_SEAN_MICRO_N+4,
	TRX_SNOW_MELT_04A_N=TRX_SNOW_MELT_02A_N+4,
	
	TABLE_FG = TRX_SNOW_MELT_04A_N+4,

	NUM_TEXTURES
};

static ID3D11ShaderResourceView * g_pTexturesHandler[NUM_TEXTURES];

enum MESH_ID
{
	MESH_QUAD,
	MESH_SPHERE,
	MESH_PIRATE,
	MESH_ROCK,
	
	NUM_MESHES
};

static CMeshDraw g_pMeshes[NUM_MESHES];

#define MODEL_PATH		"meshes/"



enum DRAWABLE_ID
{
	GROUND_EXAMPLE=0,
	SPHERE_EXAMPLE,

	PIRATE_EXAMPLE,
	ROCK_EXAMPLE,
	
	NUM_DRAWABLES
};

enum
{
	JUST_ONE_VARIANT=0,
	
	NUM_PS_VARIANTS
};


static int g_variantMode = JUST_ONE_VARIANT; 


static DWORD g_dwShaderFlags;

static CShader g_vert_shader;
static CShader g_vert_shader_basic;
static CShader g_pix_shader_basic_white;
static CShader g_pix_shader[NUM_PS_VARIANTS*NUM_DRAWABLES];

static CShaderPipeline g_ShaderPipelines_DepthOnly[NUM_DRAWABLES];
static CShaderPipeline g_ShaderPipelines[NUM_PS_VARIANTS*NUM_DRAWABLES];

static ID3D11Buffer * g_pMaterialParamsCB[NUM_DRAWABLES];
static ID3D11Buffer * g_pMeshInstanceCB[NUM_DRAWABLES];
static ID3D11Buffer * g_pMeshInstanceCB_forLabels;
static Mat44 g_mLocToWorld[NUM_DRAWABLES];
static int g_meshResourceID[NUM_DRAWABLES];

// labels are special
static CShaderPipeline g_LabelsShaderPipelines;


static ID3D11InputLayout * g_pVertexLayout = NULL;
static ID3D11InputLayout * g_pVertexSimpleLayout = NULL;


#define str(x) #x

struct SValueAndStr
{
	SValueAndStr(const int value_in, const char str_name_in[]) : value(value_in), str_name(str_name_in) {}

	const int value;
	const char * str_name;
};


#define MAKE_STR_PAIR(x)	SValueAndStr(x, str(x) )
#define MAKE_STR_SIZE_PAIR(x) SValueAndStr(sizeof(x), str(x) )

static bool GenericPipelineSetup(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB, const Mat44 &mat, const SValueAndStr &sIdxAndStr, const char pixShaderEntryFunc[], const SValueAndStr sMatSizeAndStr);
static bool GenericPipelineSetup(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB, const Mat44 &mat, const SValueAndStr &sIdxAndStr, const char pixShaderEntryFunc[])
{
	SValueAndStr defaultMatSizeAndName(0,"");
	return GenericPipelineSetup(pd3dDevice, pContext, pGlobalsCB, mat, sIdxAndStr, pixShaderEntryFunc, defaultMatSizeAndName);
}


const float g_O = 2*2.59f;
const float g_S = -1.0f;		// convert unity scene to right hand frame.

static void SetScaleAndPos(Mat44 * matPtr, const float scale, const Vec3 &pos)
{
	LoadIdentity(matPtr);
	for(int c=0; c<3; c++)
	{
		SetColumn(matPtr, c, Vec4(c==0 ? scale : 0.0f, c==1 ? scale : 0.0f, c==2 ? scale : 0.0f, 0.0f));
	}
	SetColumn(matPtr, 3, Vec4(pos.x, pos.y, pos.z, 1.0f));
}

static void SetupGroundPlanePipeline(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB)
{
	Mat44 mat; SetScaleAndPos(&mat, 30*6.0f, Vec3(g_S*3.39f+g_O, 1.28f, -0.003f) );
	GenericPipelineSetup(pd3dDevice, pContext, pGlobalsCB, mat, MAKE_STR_PAIR(GROUND_EXAMPLE), "GroundExamplePS", MAKE_STR_SIZE_PAIR(cbMatGroundShader));

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedSubResource;
	V( pContext->Map( g_pMaterialParamsCB[GROUND_EXAMPLE], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbMatGroundShader *)MappedSubResource.pData)->g_fBumpIntensity = 1.0f;
	((cbMatGroundShader *)MappedSubResource.pData)->g_fTileRate = 1.0f;
    pContext->Unmap( g_pMaterialParamsCB[GROUND_EXAMPLE], 0 );
	
	g_meshResourceID[GROUND_EXAMPLE] = MESH_QUAD;
}

static void SetupSpherePrimPipeline(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB)
{
	Mat44 mat; SetScaleAndPos(&mat, 8.0f, Vec3(g_S*3.39f+g_O - 3.5 + 6.0f, 1.28f+8.0f, -0.003f + 4.5) );
	GenericPipelineSetup(pd3dDevice, pContext, pGlobalsCB, mat, MAKE_STR_PAIR(SPHERE_EXAMPLE), "SphereExamplePS", MAKE_STR_SIZE_PAIR(cbMatSphereShader));

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedSubResource;
	V( pContext->Map( g_pMaterialParamsCB[SPHERE_EXAMPLE], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbMatSphereShader *)MappedSubResource.pData)->g_fBumpIntensity = 1.0f;
	((cbMatSphereShader *)MappedSubResource.pData)->g_fTileRate = 1.0f;
    pContext->Unmap( g_pMaterialParamsCB[SPHERE_EXAMPLE], 0 );
	
	g_meshResourceID[SPHERE_EXAMPLE] = MESH_SPHERE;
}


static void SetupPirateExamplePipeline(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB)
{
	Mat44 mat; SetScaleAndPos(&mat, 1.0f, Vec3(0.0f, 0.0f, 0.0f) );
	Mat44 rotY; LoadIdentity(&rotY);
	LoadRotation(&rotY, 0.0f, -M_PI/2.0f, 0.0f);
	mat = rotY * mat;
	SetColumn(&mat, 3, Vec4(g_S*-0.007558346f+g_O - 8, 2.237f, -2.74f+2, 1.0f));

	GenericPipelineSetup(pd3dDevice, pContext, pGlobalsCB, mat, MAKE_STR_PAIR(PIRATE_EXAMPLE), "PirateExamplePS", MAKE_STR_SIZE_PAIR(cbMatPirateShader));

	for(int i=0; i<NUM_PS_VARIANTS; i++)
	{
		g_ShaderPipelines[i*NUM_DRAWABLES+PIRATE_EXAMPLE].RegisterResourceView("g_norm_tex", g_pTexturesHandler[PIRATE_NORMALS_TS]);
		g_ShaderPipelines[i*NUM_DRAWABLES+PIRATE_EXAMPLE].RegisterResourceView("g_albedo_tex", g_pTexturesHandler[PIRATE_ALBEDO]);
		g_ShaderPipelines[i*NUM_DRAWABLES+PIRATE_EXAMPLE].RegisterResourceView("g_smoothness_tex", g_pTexturesHandler[PIRATE_SMOOTHNESS]);
		g_ShaderPipelines[i*NUM_DRAWABLES+PIRATE_EXAMPLE].RegisterResourceView("g_ao_tex", g_pTexturesHandler[PIRATE_OCCLUSION]);
		g_ShaderPipelines[i*NUM_DRAWABLES+PIRATE_EXAMPLE].RegisterResourceView("g_mask_tex", g_pTexturesHandler[PIRATE_MASK]);
	}

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedSubResource;
	V( pContext->Map( g_pMaterialParamsCB[PIRATE_EXAMPLE], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbMatPirateShader *)MappedSubResource.pData)->g_fBumpIntensity = 1.0f;
	((cbMatPirateShader *)MappedSubResource.pData)->g_fTileRate = 1.0f;
    pContext->Unmap( g_pMaterialParamsCB[PIRATE_EXAMPLE], 0 );


	g_meshResourceID[PIRATE_EXAMPLE] = MESH_PIRATE;
}

static void SetupRockPipeline(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB)
{
	Mat44 mat; SetScaleAndPos(&mat, 0.01*0.15f, Vec3(g_S*3.957f+g_O - 3.3 - 0.5, 0.119f + 2, 3.133f + 0.1f - 2) );
	const float deg2rad = M_PI/180.0f;
	Mat44 rot; LoadIdentity(&rot);

	// not identical rotation to the unity scene but doesn't matter
	LoadRotation(&rot, 0.0f*deg2rad, -90.0f*deg2rad, 0.0f*deg2rad);
	mat = mat * rot;


	GenericPipelineSetup(pd3dDevice, pContext, pGlobalsCB, mat, MAKE_STR_PAIR(ROCK_EXAMPLE), "RockExamplePS", MAKE_STR_SIZE_PAIR(cbMatRockShader));

	for(int i=0; i<NUM_PS_VARIANTS; i++)
	{
		g_ShaderPipelines[i*NUM_DRAWABLES+ROCK_EXAMPLE].RegisterResourceView("g_norm_tex", g_pTexturesHandler[ROCK_BASE_TS]);
	}

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE MappedSubResource;
	V( pContext->Map( g_pMaterialParamsCB[ROCK_EXAMPLE], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbMatRockShader *)MappedSubResource.pData)->g_fBumpIntensity = 1.0f;
	((cbMatRockShader *)MappedSubResource.pData)->g_fTileRate = 1.0f;
    pContext->Unmap( g_pMaterialParamsCB[ROCK_EXAMPLE], 0 );


	g_meshResourceID[ROCK_EXAMPLE] = MESH_ROCK;
}



static bool ImportTexture(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, const int resourceIdx, const WCHAR path[], const WCHAR name[], const bool sRGB=false)
{
	WCHAR dest_str[256];
	wcscpy(dest_str, path);
	wcscat(dest_str, name);

	HRESULT hr;

	CDXUTResourceCache &cache = DXUTGetGlobalResourceCache();
	V( cache.CreateTextureFromFile(pd3dDevice, pContext, dest_str, &g_pTexturesHandler[resourceIdx], sRGB) );

	//V_RETURN(DXUTCreateShaderResourceViewFromFile(pd3dDevice, dest_str, &g_pTexturesHandler[resourceIdx]));

	return hr==S_OK;
}

static bool ImportTextureTRX(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, const int resourceIdx, const WCHAR path[], const WCHAR name[], const bool sRGB)
{
	bool res = false;

	const int len = wcslen(name);
	int nrStrip = 0;
	while(nrStrip<5 && nrStrip<len && !res)
	{
		if(name[len-1-nrStrip]==L'.') res = true;
		else ++nrStrip;
	}

	if(res)
	{
		res &= ImportTexture(pd3dDevice, pContext, resourceIdx+0, path, name, sRGB);

		if(res)
		{
			int nrToCopy = len-1-nrStrip;
			WCHAR dest_str[256];
			WCHAR str_tmp[256];

			wcsncpy(str_tmp, name, nrToCopy);
			wcscpy(str_tmp+nrToCopy, L"");

			wcscpy(dest_str, str_tmp);
			wcscat(dest_str, L"_transfer.png");
			res &= ImportTexture(pd3dDevice, pContext, resourceIdx+1, path, dest_str);

			if(res)
			{
				wcscpy(dest_str, str_tmp);
				wcscat(dest_str, L"_invtransfer.dds");
				res &= ImportTexture(pd3dDevice, pContext, resourceIdx+2, path, dest_str);

				if(res)
				{
					wcscpy(dest_str, str_tmp);
					wcscat(dest_str, L"_basis.dds");
					res &= ImportTexture(pd3dDevice, pContext, resourceIdx+3, path, dest_str);
				}
			}
		}
	}

	return res;
}

static bool CreateNoiseData(ID3D11Device* pd3dDevice);

bool InitResources(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB)
{
	// import raw textures
	ImportTexture(pd3dDevice, pContext, PIRATE_ALBEDO, L"textures/Pirate/", L"Pirate_Albedo.png", true);
	ImportTexture(pd3dDevice, pContext, PIRATE_OCCLUSION, L"textures/Pirate/", L"Pirate_occlusion.png");
	ImportTexture(pd3dDevice, pContext, PIRATE_SMOOTHNESS, L"textures/Pirate/", L"Pirate_Smoothness.png");
	ImportTexture(pd3dDevice, pContext, PIRATE_NORMALS_TS, L"textures/Pirate/", L"Pirate_ts_normals.png");
	ImportTexture(pd3dDevice, pContext, PIRATE_MASK, L"textures/Pirate/", L"Pirate_Mask_rgb.png");
	
	ImportTexture(pd3dDevice, pContext, ROCK_BASE_TS, L"textures/Rock/", L"Rock_Overgrown_A_Normal.tif");
	
	ImportTexture(pd3dDevice, pContext, TABLE_FG, L"textures/sky/", L"tableFG.dds");

	// import tileables with histogram-preserving attachments
	ImportTextureTRX(pd3dDevice, pContext, TRX_JAGUAR, L"textures/details/", L"JaguarTile.jpg", true);
	ImportTextureTRX(pd3dDevice, pContext, TRX_GRASS_SHORT_D, L"textures/details/", L"grass_short_01a_d.png", true);
	ImportTextureTRX(pd3dDevice, pContext, TRX_MOSSGROUND_D, L"textures/details/", L"moss_ground_1k_tile.png", true);
	ImportTextureTRX(pd3dDevice, pContext, TRX_NATURE_PEBBLES_D, L"textures/details/", L"Nature_Pebbles_4K_d.png", true);

	ImportTextureTRX(pd3dDevice, pContext, TRX_PEBBLES_BEACH_CROP_N, L"textures/details/", L"pebbles_beach_crop_01a_n.png", false);
	ImportTextureTRX(pd3dDevice, pContext, TRX_SEAN_MICRO_N, L"textures/details/", L"Sean_Micro_Normal.png", false);
	ImportTextureTRX(pd3dDevice, pContext, TRX_SNOW_MELT_02A_N, L"textures/details/", L"snow_melt_02a_n.png", false);
	ImportTextureTRX(pd3dDevice, pContext, TRX_SNOW_MELT_04A_N, L"textures/details/", L"snow_melt_04a_n.png", false);
	


	// import raw mesh data
	bool res = true;
	res &= g_pMeshes[MESH_QUAD].ReadObj(pd3dDevice, MODEL_PATH "quad.obj", 1.0f, true);
	res &= g_pMeshes[MESH_SPHERE].ReadObj(pd3dDevice, MODEL_PATH "sphere.obj", 1.0f, true);
	res &= g_pMeshes[MESH_PIRATE].ReadObj(pd3dDevice, MODEL_PATH "LP_Pirate.obj", 1.0f, false);
	res &= g_pMeshes[MESH_ROCK].ReadObj(pd3dDevice, MODEL_PATH "Rock_Overgrown_A.obj", 1.0f, true);


	DWORD g_dwShaderFlags = D3D10_SHADER_OPTIMIZATION_LEVEL1;//D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    g_dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

	CONST D3D10_SHADER_MACRO* pDefines = NULL;
	g_vert_shader.CompileShaderFunction(pd3dDevice, L"shader_lighting.hlsl", pDefines, "RenderSceneVS", "vs_5_0", g_dwShaderFlags );

	// depth only pre-pass
	g_vert_shader_basic.CompileShaderFunction(pd3dDevice, L"shader_basic.hlsl", pDefines, "RenderSceneVS", "vs_5_0", g_dwShaderFlags );

	// labels shader
	g_pix_shader_basic_white.CompileShaderFunction(pd3dDevice, L"shader_basic.hlsl", pDefines, "WhitePS", "ps_5_0", g_dwShaderFlags );

	// noise data
	CreateNoiseData(pd3dDevice);
	
	HRESULT hr;
	for(int i=0; i<NUM_DRAWABLES; i++)
	{
		g_pMaterialParamsCB[i]=NULL;
		g_pMeshInstanceCB[i]=NULL;
		g_meshResourceID[i]=-1;;
		LoadIdentity(&g_mLocToWorld[i]);

		// create constant buffers
		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = (sizeof( cbMeshInstance )+0xf)&(~0xf);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateBuffer( &bd, NULL, &g_pMeshInstanceCB[i] ) );
	}

	// create labels cb
	{
		g_pMeshInstanceCB_forLabels = NULL;
		
		// create constant buffers
		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = (sizeof( cbMeshInstance )+0xf)&(~0xf);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateBuffer( &bd, NULL, &g_pMeshInstanceCB_forLabels ) );
	}


	// create vertex decleration
	const D3D11_INPUT_ELEMENT_DESC vertexlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, ATTR_OFFS(SFilVert, pos),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		0, ATTR_OFFS(SFilVert, s), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,		0, ATTR_OFFS(SFilVert, s2), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,	0, ATTR_OFFS(SFilVert, norm), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT,0, ATTR_OFFS(SFilVert, tang), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    V_RETURN( pd3dDevice->CreateInputLayout( vertexlayout, ARRAYSIZE( vertexlayout ), 
                                             g_vert_shader.GetBufferPointer(), g_vert_shader.GetBufferSize(), 
                                             &g_pVertexLayout ) );

	const D3D11_INPUT_ELEMENT_DESC simplevertexlayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, ATTR_OFFS(SFilVert, pos),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

	V_RETURN( pd3dDevice->CreateInputLayout( simplevertexlayout, ARRAYSIZE( simplevertexlayout ), 
                                             g_vert_shader_basic.GetBufferPointer(), g_vert_shader_basic.GetBufferSize(), 
                                             &g_pVertexSimpleLayout ) );


	SetupGroundPlanePipeline(pd3dDevice, pContext, pGlobalsCB);
	SetupSpherePrimPipeline(pd3dDevice, pContext, pGlobalsCB);
	SetupPirateExamplePipeline(pd3dDevice, pContext, pGlobalsCB);
	SetupRockPipeline(pd3dDevice, pContext, pGlobalsCB);
	
	for(int i=0; i<NUM_DRAWABLES; i++)
	{
		for(int j=0; j<NUM_PS_VARIANTS; j++)
		{
			g_ShaderPipelines[j*NUM_DRAWABLES+i].RegisterResourceView("g_table_FG", g_pTexturesHandler[TABLE_FG]);
		}
	}

	ToggleDetailTex(true);
	ToggleDetailTex(false);


	return res;
}

void ToggleDetailTex(bool toggleIsForColor)
{
	static int offs_d = 0;
	static int offs_n = 0;

	if(toggleIsForColor) ++offs_d;
	else ++offs_n;

	const int indices_d[] =
	{
		TRX_GRASS_SHORT_D,
		TRX_NATURE_PEBBLES_D,
		TRX_MOSSGROUND_D,
		TRX_JAGUAR
	};

	const int indices_n[] =
	{
		TRX_PEBBLES_BEACH_CROP_N,
		TRX_SEAN_MICRO_N,
		TRX_SNOW_MELT_02A_N,
		TRX_SNOW_MELT_04A_N
	};

	const int nrColorTex = sizeof(indices_d) / sizeof(int);
	const int nrNormalTex = sizeof(indices_n) / sizeof(int);


	for(int i=0; i<NUM_DRAWABLES; i++)
	{
		const int idxD = (offs_d+i)%nrColorTex;
		const int idxN = (offs_n+i)%nrNormalTex;
		const int idx = toggleIsForColor ? indices_d[idxD] : indices_n[idxN];

		for(int j=0; j<NUM_PS_VARIANTS; j++)
		{
			g_ShaderPipelines[j*NUM_DRAWABLES+i].RegisterResourceView(toggleIsForColor ? "g_trx_d" : "g_trx_n", g_pTexturesHandler[idx+0]);
			g_ShaderPipelines[j*NUM_DRAWABLES+i].RegisterResourceView(toggleIsForColor ? "g_trx_transfer_d" : "g_trx_transfer_n", g_pTexturesHandler[idx+1]);
			g_ShaderPipelines[j*NUM_DRAWABLES+i].RegisterResourceView(toggleIsForColor ? "g_trx_invtransfer_d" : "g_trx_invtransfer_n", g_pTexturesHandler[idx+2]);
			g_ShaderPipelines[j*NUM_DRAWABLES+i].RegisterResourceView(toggleIsForColor ? "g_trx_basis_d" : "g_trx_basis_n", g_pTexturesHandler[idx+3]);
		}
	}
}

void PassShadowResolve(ID3D11ShaderResourceView * pShadowResolveSRV)
{
	for(int i=0; i<NUM_DRAWABLES; i++)
	{
		for(int j=0; j<NUM_PS_VARIANTS; j++)
		{
			if(pShadowResolveSRV!=NULL) 
				g_ShaderPipelines[j*NUM_DRAWABLES+i].RegisterResourceView("g_shadowResolve", pShadowResolveSRV);
		}
	}
}

static void RegisterGenericNoiseBuffers(CShaderPipeline &pipe);

static bool GenericPipelineSetup(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB, const Mat44 &mat, const SValueAndStr &sIdxAndStr, const char pixShaderEntryFunc[], const SValueAndStr sMatSizeAndStr)
{
	const int drawableIdx = sIdxAndStr.value; 

	for(int i=0; i<NUM_PS_VARIANTS; i++)
	{
		//const char decals_enabled[] = "DECALS_ENABLED";
		//const char decals_mip_mapped[] = "DECALS_MIP_MAPPED";

		//const bool haveDecals = i==DECALS_ENABLED_MIPMAPPED_ON || i==DECALS_ENABLED_MIPMAPPED_OFF;
		//CONST D3D10_SHADER_MACRO sDefines[] = {{sIdxAndStr.str_name, NULL}, {haveDecals ? decals_enabled : NULL, NULL}, {i==DECALS_ENABLED_MIPMAPPED_ON ? decals_mip_mapped : NULL, NULL}, {NULL, NULL}};
		CONST D3D10_SHADER_MACRO sDefines[] = {{sIdxAndStr.str_name, NULL}, {NULL, NULL}};

		g_pix_shader[i*NUM_DRAWABLES+drawableIdx].CompileShaderFunction(pd3dDevice, L"shader_lighting.hlsl", sDefines, pixShaderEntryFunc, "ps_5_0", g_dwShaderFlags );

		CShaderPipeline &pipe = g_ShaderPipelines[i*NUM_DRAWABLES+drawableIdx];

		// prepare shader pipeline
		pipe.SetVertexShader(&g_vert_shader);
		pipe.SetPixelShader(&g_pix_shader[i*NUM_DRAWABLES+drawableIdx]);

		// register constant buffers
		pipe.RegisterConstBuffer("cbMeshInstance", g_pMeshInstanceCB[drawableIdx]);
		pipe.RegisterConstBuffer("cbGlobals", pGlobalsCB);
		RegisterGenericNoiseBuffers(pipe);
	
		// register samplers
		pipe.RegisterSampler("g_samWrapAniso", GetDefaultSamplerWrapAniso() );
		pipe.RegisterSampler("g_samWrap", GetDefaultSamplerWrap() );
		pipe.RegisterSampler("g_samClamp", GetDefaultSamplerClamp() );
		pipe.RegisterSampler("g_samShadow", GetDefaultShadowSampler() );

	}
	   

	CShaderPipeline &pipeDepth = g_ShaderPipelines_DepthOnly[drawableIdx];

	

	// depth only pipeline
	pipeDepth.SetVertexShader(&g_vert_shader_basic);
	pipeDepth.RegisterConstBuffer("cbMeshInstance", g_pMeshInstanceCB[drawableIdx]);
	pipeDepth.RegisterConstBuffer("cbGlobals", pGlobalsCB);

	HRESULT hr;

	// create constant buffers
	const int byteSizeMatCB = sMatSizeAndStr.value;
	if(byteSizeMatCB>0)
	{
		D3D11_BUFFER_DESC bd;

		memset(&bd, 0, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = (byteSizeMatCB+0xf)&(~0xf);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		V_RETURN( pd3dDevice->CreateBuffer( &bd, NULL, &g_pMaterialParamsCB[drawableIdx] ) );

		for(int i=0; i<NUM_PS_VARIANTS; i++)
		{
			CShaderPipeline &pipe = g_ShaderPipelines[i*NUM_DRAWABLES+drawableIdx];
			pipe.RegisterConstBuffer(sMatSizeAndStr.str_name, g_pMaterialParamsCB[drawableIdx]);
		}
		pipeDepth.RegisterConstBuffer(sMatSizeAndStr.str_name, g_pMaterialParamsCB[drawableIdx]);
	}

	// update transformation cb
	D3D11_MAPPED_SUBRESOURCE MappedSubResource;
	V( pContext->Map( g_pMeshInstanceCB[drawableIdx], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubResource ) );
	((cbMeshInstance *)MappedSubResource.pData)->g_mLocToWorld = Transpose(mat); 
	((cbMeshInstance *)MappedSubResource.pData)->g_mWorldToLocal = Transpose(~mat); 
    pContext->Unmap( g_pMeshInstanceCB[drawableIdx], 0 );

	// make a record
	g_mLocToWorld[drawableIdx] = mat;

	return true;
}


void RenderSceneGraph(ID3D11DeviceContext *pContext, bool bSimpleLayout, bool bSkipGroundPlane)
{
	for(int i=0; i<NUM_DRAWABLES; i++)
	{
		if((!bSkipGroundPlane) || i!=GROUND_EXAMPLE)
		{
			CShaderPipeline &pipe = g_ShaderPipelines[0*NUM_DRAWABLES+i];

			pipe.PrepPipelineForRendering(pContext);
	
			CMeshDraw &mesh = g_pMeshes[ g_meshResourceID[i] ];

			// set streams and layout
			UINT stride = sizeof(SFilVert), offset = 0;
			pContext->IASetVertexBuffers( 0, 1, mesh.GetVertexBuffer(), &stride, &offset );
			pContext->IASetIndexBuffer( mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0 );
			pContext->IASetInputLayout( bSimpleLayout ? g_pVertexSimpleLayout : g_pVertexLayout );

			// Set primitive topology
			pContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
			pContext->DrawIndexed( 3*mesh.GetNrTriangles(), 0, 0 );
			pipe.FlushResources(pContext);
		}
	}
}


bool InitializeSceneGraph(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB)
{
	bool res = InitResources(pd3dDevice, pContext, pGlobalsCB);

	// make the sun
	Mat33 rotX, rotY;
	LoadRotation(&rotX, 30.774*(M_PI/180), 0, 0.0f);
	LoadRotation(&rotY, 0, -30*(M_PI/180), 0.0f);

	Mat33 sunRot2 = rotX * rotY;		// the unity order. Rotates around Y first, then X.
	Mat33 sunRot3 = rotY * rotX;

	// in Lys Azi is 30, Zeni is 31, Y is up and no flips

	//Vec3 g_vSunDir = -Normalize(Vec3(-2.0f,2.0f,-2.5f));
	//Vec3 g_vSunDir = GetColumn(sunRot2, 2); g_vSunDir.x=-g_vSunDir.x;		// matches direction in Unity sample
	g_vSunDir = GetColumn(sunRot3, 2); g_vSunDir.x=-g_vSunDir.x;

	g_vSunDir.y -= 0.04f;
	g_vSunDir = Normalize(g_vSunDir);


	return res;
}

Vec3 GetSunDir()
{
	return g_vSunDir;
}

static CBufferObject g_PermTableBuffer;
static CBufferObject g_GradBuffer;

void ReleaseSceneGraph()
{
	for(int t=0; t<NUM_TEXTURES; t++)
		SAFE_RELEASE( g_pTexturesHandler[t] );

	for(int m=0; m<NUM_MESHES; m++)
		g_pMeshes[m].CleanUp();

	CDXUTResourceCache &cache = DXUTGetGlobalResourceCache();
	cache.OnDestroyDevice();

	g_vert_shader.CleanUp();
	g_vert_shader_basic.CleanUp();
	g_pix_shader_basic_white.CleanUp();

	for(int i=0; i<NUM_DRAWABLES; i++)
	{
		SAFE_RELEASE( g_pMeshInstanceCB[i] );
		if(g_pMaterialParamsCB[i]!=NULL) SAFE_RELEASE( g_pMaterialParamsCB[i] );
		for(int j=0; j<NUM_PS_VARIANTS; j++)
			g_pix_shader[j*NUM_DRAWABLES+i].CleanUp();
	}
	SAFE_RELEASE( g_pMeshInstanceCB_forLabels );

	SAFE_RELEASE( g_pVertexLayout );
	SAFE_RELEASE( g_pVertexSimpleLayout );

	g_GradBuffer.CleanUp();
	g_PermTableBuffer.CleanUp();
}




static bool CreateNoiseData(ID3D11Device* pd3dDevice)
{
	// 3D version
	static const unsigned char uPermTable[256] =
	{
		151,160,137,91,90,15,
		131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
		190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
		88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
		77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
		102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
		135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
		5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
		223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
		129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
		251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
		49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
		138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
	};

	static const Vec3 grads_array[] =
	{
		Vec3(1,1,0), Vec3(-1,1,0), Vec3(1,-1,0), Vec3(-1,-1,0),
		Vec3(1,0,1), Vec3(-1,0,1), Vec3(1,0,-1), Vec3(-1,0,-1),
		Vec3(0,1,1), Vec3(0,-1,1), Vec3(0,1,-1), Vec3(0,-1,-1),
		Vec3(1,1,0), Vec3(0,-1,1), Vec3(-1,1,0), Vec3(0,-1,-1)
	};

	bool res = true;
	res &= g_PermTableBuffer.CreateBuffer(pd3dDevice, 256, 0, uPermTable, CBufferObject::DefaultBuf, true, false);
	res &= g_PermTableBuffer.AddTypedSRV(pd3dDevice, DXGI_FORMAT_R8_UINT);

	res &= g_GradBuffer.CreateBuffer(pd3dDevice, 16*sizeof(Vec3), 0, grads_array, CBufferObject::DefaultBuf, true, false);
	res &= g_GradBuffer.AddTypedSRV(pd3dDevice, DXGI_FORMAT_R32G32B32_FLOAT);

	return res;
}


static void RegisterGenericNoiseBuffers(CShaderPipeline &pipe)
{
	pipe.RegisterResourceView("g_uPermTable", g_PermTableBuffer.GetSRV());
	pipe.RegisterResourceView("g_v3GradArray", g_GradBuffer.GetSRV());
}


// shadow support functions
int GetNumberOfShadowCastingMeshInstances()
{
	return NUM_DRAWABLES-1;		// minus one to exclude the groundplane
}

void GetAABBoxAndTransformOfShadowCastingMeshInstance(Vec3 * pvMin, Vec3 * pvMax, Mat44 * pmMat, const int idx_in)
{
	const int idx = idx_in+1;		// skip groundplane
	assert(idx>=0 && idx<NUM_DRAWABLES);

	const int mesh_idx = g_meshResourceID[idx];


	CMeshDraw &mesh = g_pMeshes[mesh_idx];

	*pvMin = mesh.GetMin();
	*pvMax = mesh.GetMax();
	*pmMat = g_mLocToWorld[idx];
}