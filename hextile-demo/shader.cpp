#include "shader.h"
#include "SDKmisc.h"


int CShader::GetResourceBufferSlot(const char cResourceName[])
{
	HRESULT     hr = NOERROR;

	ID3D11ShaderReflection* pReflector = NULL; 
	V( D3DReflect( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**) &pReflector) );
	
	void * ptr0 = m_pShaderBlob;
	void * ptr1 = m_pShaderBlob->GetBufferPointer();
	unsigned int uSize = m_pShaderBlob->GetBufferSize();


	D3D11_SHADER_INPUT_BIND_DESC sDesc;
	hr = pReflector->GetResourceBindingDescByName(cResourceName, &sDesc);
	const int iSlotIndex = hr != NOERROR ? -1 : sDesc.BindPoint;
	pReflector->Release();

	return iSlotIndex;
}



void CShader::CompileShaderFunction( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D10_SHADER_MACRO* pDefines, 
                                    LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1 )
{
	HRESULT     hr = NOERROR;
	BOOL bDumpShader = false;
	UINT Flags2 = 0;
	LPD3D10INCLUDE pInclude = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    V( CreateShaderFromFile( pd3dDevice, pSrcFile, pDefines, pInclude, pFunctionName, pProfile, Flags1, Flags2, &m_pShader, &m_pShaderBlob, bDumpShader ) );
}


// this function came from a d3d11 SDK sample
HRESULT CShader::CreateShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D10_SHADER_MACRO* pDefines, 
                              LPD3D10INCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                              ID3D11DeviceChild** ppShader, ID3D10Blob** ppShaderBlob, 
                              BOOL bDumpShader)
{
	HRESULT     hr = NOERROR;
    ID3D10Blob* pShaderBlob = NULL;
    ID3D10Blob* pErrorBlob = NULL;
	m_eShaderType = eUNDEFINED;


    WCHAR wcFullPath[256];
    DXUTFindDXSDKMediaFileCch( wcFullPath, 256, pSrcFile );
    // Compile shader into binary blob
	hr = D3DCompileFromFile(wcFullPath, pDefines, pInclude, pFunctionName, pProfile,
                                Flags1, Flags2, &pShaderBlob, &pErrorBlob);
	
    if( FAILED( hr ) )
    {
        OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
        SAFE_RELEASE( pErrorBlob );
        return hr;
    }
    
    // Create shader from binary blob
    if ( ppShader!=NULL )
    {
        hr = E_FAIL;
        if ( strstr( pProfile, "vs" ) )
        {
            hr = pd3dDevice->CreateVertexShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11VertexShader**)ppShader );
			m_eShaderType = eVSHADER;
        }
        else if ( strstr( pProfile, "hs" ) )
        {
            hr = pd3dDevice->CreateHullShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11HullShader**)ppShader ); 
			m_eShaderType = eHSHADER;
        }
        else if ( strstr( pProfile, "ds" ) )
        {
            hr = pd3dDevice->CreateDomainShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11DomainShader**)ppShader );
			m_eShaderType = eDSHADER;
        }
        else if ( strstr( pProfile, "gs" ) )
        {
            hr = pd3dDevice->CreateGeometryShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11GeometryShader**)ppShader ); 
			m_eShaderType = eGSHADER;
        }
        else if ( strstr( pProfile, "ps" ) )
        {
            hr = pd3dDevice->CreatePixelShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11PixelShader**)ppShader );
			m_eShaderType = ePSHADER;
        }
        else if ( strstr( pProfile, "cs" ) )
        {
			const void *pShaderBytecode = pShaderBlob->GetBufferPointer();
			const SIZE_T BytecodeLength = pShaderBlob->GetBufferSize();
			ID3D11ComputeShader** bla = (ID3D11ComputeShader**)ppShader;

            hr = pd3dDevice->CreateComputeShader( pShaderBlob->GetBufferPointer(), 
                    pShaderBlob->GetBufferSize(), NULL, (ID3D11ComputeShader**)ppShader );
			m_eShaderType = eCSHADER;
        }
        if ( FAILED( hr ) )
        {
            OutputDebugString( L"Shader creation failed\n" );
            SAFE_RELEASE( pErrorBlob );
            SAFE_RELEASE( pShaderBlob );
            return hr;
        }
    }

    // If blob was requested then pass it otherwise release it
    if ( ppShaderBlob!=NULL )
    {
        *ppShaderBlob = pShaderBlob;
    }
    else
    {
        pShaderBlob->Release();
    }

    // Return error code
    return hr;
}


bool CShader::CreateBlobFromBinaryFile(const char file[])
{
	bool bRes = false;
	FILE * fptr_in = fopen(file, "rb");
	if(fptr_in!=NULL)
	{
		fseek(fptr_in, 0, SEEK_END);
		long size = ftell(fptr_in);
		fseek(fptr_in, 0, SEEK_SET);
		long start = ftell(fptr_in);
		assert(start==0);
		HRESULT hr = D3DCreateBlob(size-start, &m_pShaderBlob);
		if(m_pShaderBlob!=NULL)
		{
			fread(m_pShaderBlob->GetBufferPointer(), 1, m_pShaderBlob->GetBufferSize(), fptr_in);
			bRes=true;
		}

		fclose(fptr_in);
	}

	return bRes;
}

void CShader::CreateComputeShaderFromBinary(ID3D11Device* pd3dDevice, const char filename[])
{
	HRESULT     hr = E_FAIL; //D3D_OK
	m_eShaderType = eUNDEFINED;

	bool bRes = CreateBlobFromBinaryFile(filename);

	if(bRes && m_pShaderBlob!=NULL)
	{
		

		V( pd3dDevice->CreateComputeShader( m_pShaderBlob->GetBufferPointer(), m_pShaderBlob->GetBufferSize(), NULL, (ID3D11ComputeShader**)&m_pShader ) );
		m_eShaderType = eCSHADER;
	}
    
    if ( !bRes || FAILED( hr ) )
    {
        OutputDebugString( L"Shader creation failed\n" );
    }
}

void CShader::CleanUp()
{
	SAFE_RELEASE( m_pShader );
	SAFE_RELEASE( m_pShaderBlob );
}



CShader::CShader()
{
	m_eShaderType = eUNDEFINED;
	m_pShaderBlob=NULL;
}

CShader::~CShader()
{
	m_eShaderType = eUNDEFINED;
}