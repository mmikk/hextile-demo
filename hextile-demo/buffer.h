#ifndef __BUFFEROBJ_H__
#define __BUFFEROBJ_H__

//#include <d3dx11.h>
#include <d3d11_2.h> 

class CBufferObject
{
public:
	enum EMiscFlags
	{
		DefaultBuf=0,
		StructuredBuf,
		RawViewBuf
	};

	enum EStagingBuf
	{
		NoStaging=0,
		StagingCpuReadOnly,
		StagingCpuWriteOnly,
		StagingCpuReadWrite
	};

	bool AddTypedSRV(ID3D11Device* pd3dDevice, DXGI_FORMAT format);
	bool AddStructuredSRV(ID3D11Device* pd3dDevice);
	bool AddTypedUAV(ID3D11Device* pd3dDevice, DXGI_FORMAT format);
	bool AddStructuredUAV(ID3D11Device* pd3dDevice);
	bool CreateBuffer(ID3D11Device* pd3dDevice, const int totalByteSize, const int structuredByteSize, const void * pInitData, EMiscFlags eFlags, const bool bEnableSRV=true, const bool bEnableUAV=false, EStagingBuf eStageFlags=NoStaging);
	
	ID3D11Buffer * GetBuffer();
	ID3D11Buffer * GetStagedBuffer();
	ID3D11ShaderResourceView * GetSRV(const int idx=0);
	ID3D11UnorderedAccessView * GetUAV(const int idx=0);

	void CleanUp();

	CBufferObject();
	~CBufferObject();
private:
	bool AddSRV_Internal(ID3D11Device* pd3dDevice, DXGI_FORMAT format, const int numEntries);
	bool AddUAV_Internal(ID3D11Device* pd3dDevice, DXGI_FORMAT format, const int numEntries);

	int m_iNrSRVs, m_iNrUAVs;
	ID3D11Buffer * m_mem, * m_mem_staged;
	ID3D11ShaderResourceView ** m_ppSRVs;
	ID3D11UnorderedAccessView ** m_ppUAVs;
	EMiscFlags m_eFlags;

	int m_iBufferByteSize, m_iStructureByteSize;
};

#endif