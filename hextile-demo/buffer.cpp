#include "buffer.h"
#include "shaderutils.h"
#include "DXUT.h"
#include <assert.h>

bool CBufferObject::CreateBuffer(ID3D11Device* pd3dDevice, const int totalByteSize, const int structuredByteSize, const void * pInitData, EMiscFlags eFlags, const bool bEnableSRV, const bool bEnableUAV, EStagingBuf eStageFlags)
{
	CleanUp();

	D3D11_SUBRESOURCE_DATA sData;
	sData.pSysMem = pInitData;
	sData.SysMemPitch = 0;
	sData.SysMemSlicePitch = 0;

	D3D11_SUBRESOURCE_DATA * pData = pInitData==NULL ? NULL : &sData;

	
	D3D11_BUFFER_DESC bd;
	memset(&bd, 0, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = (bEnableSRV ? D3D11_BIND_SHADER_RESOURCE : 0) | (bEnableUAV ? D3D11_BIND_UNORDERED_ACCESS : 0);
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = eFlags==StructuredBuf ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : (eFlags==RawViewBuf ? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : 0);
	bd.ByteWidth = totalByteSize;
	bd.StructureByteStride = structuredByteSize;
	HRESULT vr = pd3dDevice->CreateBuffer( &bd, pData, &m_mem );

	if(eStageFlags!=NoStaging)
	{
		const bool bStageBufCpuRead = eStageFlags==StagingCpuReadOnly || eStageFlags==StagingCpuReadWrite;
		const bool bStageBufCpuWrite = eStageFlags==StagingCpuWriteOnly || eStageFlags==StagingCpuReadWrite;

		bd.Usage = D3D11_USAGE_STAGING;
		bd.BindFlags = 0;
		bd.CPUAccessFlags = (bStageBufCpuRead ? D3D11_CPU_ACCESS_READ : 0) | (bStageBufCpuWrite ? D3D11_CPU_ACCESS_WRITE : 0);
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		vr = pd3dDevice->CreateBuffer( &bd, NULL, &m_mem_staged );
	}

	m_eFlags = eFlags;
	m_iBufferByteSize = totalByteSize;
	m_iStructureByteSize = structuredByteSize;

	return true;
}

bool CBufferObject::AddStructuredSRV(ID3D11Device* pd3dDevice)
{
	assert(m_eFlags==StructuredBuf);
	const int numEntries = (m_eFlags==StructuredBuf && m_iStructureByteSize>0) ? (m_iBufferByteSize/m_iStructureByteSize) : 0;
	return AddSRV_Internal(pd3dDevice, DXGI_FORMAT_UNKNOWN, numEntries);
}
bool CBufferObject::AddTypedSRV(ID3D11Device* pd3dDevice, DXGI_FORMAT format)
{
	assert(format!=DXGI_FORMAT_UNKNOWN);
	assert(m_eFlags==DefaultBuf);
	int fmt_size = (int) (GetDXGIBitWidth(format)/8);
	const int numEntries = (m_eFlags==DefaultBuf && fmt_size>0) ? (m_iBufferByteSize/fmt_size) : 0;
	return AddSRV_Internal(pd3dDevice, format, numEntries);
}

bool CBufferObject::AddStructuredUAV(ID3D11Device* pd3dDevice)
{
	assert(m_eFlags==StructuredBuf);
	const int numEntries = (m_eFlags==StructuredBuf && m_iStructureByteSize>0) ? (m_iBufferByteSize/m_iStructureByteSize) : 0;
	return AddUAV_Internal(pd3dDevice, DXGI_FORMAT_UNKNOWN, numEntries);
}
bool CBufferObject::AddTypedUAV(ID3D11Device* pd3dDevice, DXGI_FORMAT format)
{
	assert(format!=DXGI_FORMAT_UNKNOWN);
	assert(m_eFlags==DefaultBuf);
	int fmt_size = (int) (GetDXGIBitWidth(format)/8);
	const int numEntries = (m_eFlags==DefaultBuf && fmt_size>0) ? (m_iBufferByteSize/fmt_size) : 0;
	return AddUAV_Internal(pd3dDevice, format, numEntries);
}

bool CBufferObject::AddSRV_Internal(ID3D11Device* pd3dDevice, DXGI_FORMAT format, const int numEntries)
{
	bool bRes = false;
	assert((m_iNrSRVs==0 && m_ppSRVs==NULL) || (m_iNrSRVs!=0 && m_ppSRVs!=NULL));
	assert(m_mem!=NULL && numEntries>0);

	if(m_mem!=NULL && numEntries>0)
	{
		int iLen = m_iNrSRVs>=0 ? (m_iNrSRVs+1) : 1;
		ID3D11ShaderResourceView ** ppSRVs = new ID3D11ShaderResourceView *[iLen];
		if(ppSRVs!=NULL)
		{
			if(m_ppSRVs!=NULL) 
			{ 
				for(int i=0; i<(iLen-1); i++) { ppSRVs[i]=m_ppSRVs[i]; }
				delete [] m_ppSRVs; m_ppSRVs=NULL; 
			}
			m_ppSRVs=ppSRVs; ppSRVs=NULL; m_iNrSRVs=iLen; 

			// create the SRV
			m_ppSRVs[m_iNrSRVs-1]=NULL;

			D3D11_SHADER_RESOURCE_VIEW_DESC srvbuffer_desc;
			ZeroMemory( &srvbuffer_desc, sizeof(srvbuffer_desc) );
			srvbuffer_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;		// D3D11_SRV_DIMENSION_BUFFEREX
			//srvbuffer_desc.Buffer.ElementWidth = desc.ByteWidth / elemSize;

			srvbuffer_desc.Buffer.NumElements = numEntries;
			srvbuffer_desc.Format = format;
			HRESULT hr = pd3dDevice->CreateShaderResourceView( m_mem, &srvbuffer_desc, &m_ppSRVs[m_iNrSRVs-1] );
			bRes = hr==S_OK;
		}
	}

	return bRes;
}

bool CBufferObject::AddUAV_Internal(ID3D11Device* pd3dDevice, DXGI_FORMAT format, const int numEntries)
{
	bool bRes = false;
	assert((m_iNrUAVs==0 && m_ppUAVs==NULL) || (m_iNrUAVs!=0 && m_ppUAVs!=NULL));
	assert(m_mem!=NULL && numEntries>0);

	if(m_mem!=NULL && numEntries>0)
	{
		int iLen = m_iNrUAVs>=0 ? (m_iNrUAVs+1) : 1;
		ID3D11UnorderedAccessView ** ppUAVs = new ID3D11UnorderedAccessView *[iLen];
		if(ppUAVs!=NULL)
		{
			if(m_ppUAVs!=NULL) 
			{ 
				for(int i=0; i<(iLen-1); i++) { ppUAVs[i]=m_ppUAVs[i]; }
				delete [] m_ppUAVs; m_ppUAVs=NULL; 
			}
			m_ppUAVs=ppUAVs; ppUAVs=NULL; m_iNrUAVs=iLen; 

			// create the UAV
			m_ppUAVs[m_iNrUAVs-1]=NULL;

			D3D11_UNORDERED_ACCESS_VIEW_DESC uavbuffer_desc;
			ZeroMemory( &uavbuffer_desc, sizeof(uavbuffer_desc) );

			uavbuffer_desc.Format = format;
			uavbuffer_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavbuffer_desc.Buffer.NumElements = numEntries;
	
			HRESULT hr = pd3dDevice->CreateUnorderedAccessView( m_mem, &uavbuffer_desc, &m_ppUAVs[m_iNrUAVs-1] );
			bRes = hr==S_OK;
		}
	}

	return bRes;
}

ID3D11ShaderResourceView * CBufferObject::GetSRV(const int idx)
{
	ID3D11ShaderResourceView * pRes = NULL;

	assert(idx>=0 && idx<m_iNrSRVs);
	if(idx>=0 && idx<m_iNrSRVs) pRes = m_ppSRVs[idx];
	
	return pRes;
}

ID3D11UnorderedAccessView * CBufferObject::GetUAV(const int idx)
{
	ID3D11UnorderedAccessView * pRes = NULL;

	assert(idx>=0 && idx<m_iNrUAVs);
	if(idx>=0 && idx<m_iNrUAVs) pRes = m_ppUAVs[idx];
	
	return pRes;
}

ID3D11Buffer * CBufferObject::GetBuffer()
{
	return m_mem;
}

ID3D11Buffer * CBufferObject::GetStagedBuffer()
{
	return m_mem_staged;
}

void CBufferObject::CleanUp()
{
	m_iBufferByteSize = 0;
	m_iStructureByteSize = 0;
	m_eFlags=DefaultBuf;
	if(m_mem!=NULL) SAFE_RELEASE(m_mem);
	if(m_mem_staged!=NULL) SAFE_RELEASE(m_mem_staged);
	for(int i=0; i<m_iNrSRVs; i++)
	{ SAFE_RELEASE(m_ppSRVs[i]); }
	for(int i=0; i<m_iNrUAVs; i++)
	{ SAFE_RELEASE(m_ppUAVs[i]); }

	if(m_ppSRVs!=NULL) { delete [] m_ppSRVs; m_ppSRVs=NULL; }
	if(m_ppUAVs!=NULL) { delete [] m_ppUAVs; m_ppUAVs=NULL; }
	m_iNrSRVs=0; m_iNrUAVs=0;
}

CBufferObject::CBufferObject()
{
	m_iNrSRVs=0; m_iNrUAVs=0;
	m_mem=NULL; m_mem_staged=NULL;
	m_ppSRVs=NULL; m_ppUAVs=NULL;
	m_eFlags=DefaultBuf;

	m_iBufferByteSize = 0;
	m_iStructureByteSize = 0;
}

CBufferObject::~CBufferObject()
{

}