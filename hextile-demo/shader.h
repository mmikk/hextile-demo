#ifndef __SHADER_H__
#define __SHADER_H__


#include "DXUT.h"
#include <d3d11_2.h> 



class CShader
{
public:
	ID3D11DeviceChild * GetDeviceChild() { return m_pShader; }
	void CompileShaderFunction( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D10_SHADER_MACRO* pDefines, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1 );
	void CreateComputeShaderFromBinary(ID3D11Device* pd3dDevice, const char filename[]);
	int GetResourceBufferSlot(const char cResourceName[]);
	
	LPVOID GetBufferPointer() { return m_pShaderBlob->GetBufferPointer(); }
	SIZE_T GetBufferSize() { return m_pShaderBlob->GetBufferSize(); }

	void CleanUp();


	CShader();
	~CShader();


private:
	enum
	{
		eVSHADER=0,
		eHSHADER,
		eDSHADER,
		eGSHADER,
		ePSHADER,
		eCSHADER,

		eUNDEFINED
	} m_eShaderType;

	ID3D11DeviceChild * m_pShader;
	ID3DBlob * m_pShaderBlob;


private:
	HRESULT CreateShaderFromFile( ID3D11Device* pd3dDevice, LPCWSTR pSrcFile, CONST D3D10_SHADER_MACRO* pDefines, 
                              LPD3D10INCLUDE pInclude, LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, 
                              ID3D11DeviceChild** ppShader, ID3D10Blob** ppShaderBlob, 
                              BOOL bDumpShader);

	bool CreateBlobFromBinaryFile(const char file[]);
};

#endif