#include "shaderpipeline.h"
#include "shader.h"
#include <assert.h>

// prepare pipeline for drawing
void CShaderPipeline::PrepPipelineForRendering(ID3D11DeviceContext* pd3dImmCntxt)
{
	ID3D11VertexShader * pVShader = NULL;
	ID3D11HullShader * pHShader = NULL;
	ID3D11DomainShader * pDShader = NULL;
	ID3D11GeometryShader * pGShader = NULL;
	ID3D11PixelShader * pPShader = NULL;


	int iNr = 0;
	if(m_sVertShader.pShader!=NULL)	// Vertex shader
	{
		iNr = m_sVertShader.sCnstBufferList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->VSSetConstantBuffers( 0, iNr, (ID3D11Buffer **) m_sVertShader.sCnstBufferList.m_pList);	
		iNr = m_sVertShader.sSamplerList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->VSSetSamplers( 0, iNr, (ID3D11SamplerState **) m_sVertShader.sSamplerList.m_pList);	
		iNr = m_sVertShader.sResourceViewList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->VSSetShaderResources( 0, iNr, (ID3D11ShaderResourceView **) m_sVertShader.sResourceViewList.m_pList);
		pVShader = (ID3D11VertexShader *) m_sVertShader.pShader->GetDeviceChild();
	}

	if(m_sHullShader.pShader!=NULL)	// Hull shader
	{
		iNr = m_sHullShader.sCnstBufferList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->HSSetConstantBuffers( 0, iNr, (ID3D11Buffer **) m_sHullShader.sCnstBufferList.m_pList);	
		iNr = m_sHullShader.sSamplerList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->HSSetSamplers( 0, iNr, (ID3D11SamplerState **) m_sHullShader.sSamplerList.m_pList);	
		iNr = m_sHullShader.sResourceViewList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->HSSetShaderResources( 0, iNr, (ID3D11ShaderResourceView **) m_sHullShader.sResourceViewList.m_pList);
		pHShader = (ID3D11HullShader *) m_sHullShader.pShader->GetDeviceChild();
	}

	if(m_sDomainShader.pShader!=NULL)	// Domain shader
	{
		iNr = m_sDomainShader.sCnstBufferList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->DSSetConstantBuffers( 0, iNr, (ID3D11Buffer **) m_sDomainShader.sCnstBufferList.m_pList);	
		iNr = m_sDomainShader.sSamplerList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->DSSetSamplers( 0, iNr, (ID3D11SamplerState **) m_sDomainShader.sSamplerList.m_pList);	
		iNr = m_sDomainShader.sResourceViewList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->DSSetShaderResources( 0, iNr, (ID3D11ShaderResourceView **) m_sDomainShader.sResourceViewList.m_pList);
		pDShader = (ID3D11DomainShader *) m_sDomainShader.pShader->GetDeviceChild();
	}

	if(m_sGeometryShader.pShader!=NULL)	// Geometry shader
	{
		iNr = m_sGeometryShader.sCnstBufferList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->GSSetConstantBuffers( 0, iNr, (ID3D11Buffer **) m_sGeometryShader.sCnstBufferList.m_pList);	
		iNr = m_sGeometryShader.sSamplerList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->GSSetSamplers( 0, iNr, (ID3D11SamplerState **) m_sGeometryShader.sSamplerList.m_pList);	
		iNr = m_sGeometryShader.sResourceViewList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->GSSetShaderResources( 0, iNr, (ID3D11ShaderResourceView **) m_sGeometryShader.sResourceViewList.m_pList);
		pGShader = (ID3D11GeometryShader *) m_sGeometryShader.pShader->GetDeviceChild();
	}

	if(m_sPixelShader.pShader!=NULL)	// Pixel shader
	{
		iNr = m_sPixelShader.sCnstBufferList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->PSSetConstantBuffers( 0, iNr, (ID3D11Buffer **) m_sPixelShader.sCnstBufferList.m_pList);	
		iNr = m_sPixelShader.sSamplerList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->PSSetSamplers( 0, iNr, (ID3D11SamplerState **) m_sPixelShader.sSamplerList.m_pList);	
		iNr = m_sPixelShader.sResourceViewList.iAllocatedArraySize;
		if(iNr>0) pd3dImmCntxt->PSSetShaderResources( 0, iNr, (ID3D11ShaderResourceView **) m_sPixelShader.sResourceViewList.m_pList);
		pPShader = (ID3D11PixelShader *) m_sPixelShader.pShader->GetDeviceChild();
	}

	pd3dImmCntxt->VSSetShader( pVShader, NULL, 0 );
	pd3dImmCntxt->HSSetShader( pHShader, NULL, 0 );
	pd3dImmCntxt->DSSetShader( pDShader, NULL, 0 );
	pd3dImmCntxt->GSSetShader( pGShader, NULL, 0 );
	pd3dImmCntxt->PSSetShader( pPShader, NULL, 0 );
}

#define MAX_CLEAR		8

void CShaderPipeline::FlushResources(ID3D11DeviceContext* pd3dImmCntxt)
{
	ID3D11Buffer * const buf_clear[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	ID3D11SamplerState * const sampler_clear[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	ID3D11ShaderResourceView * const srv_clear[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

	int iNr = 0;
	if(m_sVertShader.pShader!=NULL)	// Vertex shader
	{
		iNr = m_sVertShader.sCnstBufferList.iAllocatedArraySize;
		for(int b=0; b<iNr; b+= MAX_CLEAR) pd3dImmCntxt->VSSetConstantBuffers( b, min(iNr-b, MAX_CLEAR), buf_clear);	
		iNr = m_sVertShader.sSamplerList.iAllocatedArraySize;
		for(int s=0; s<iNr; s+= MAX_CLEAR) pd3dImmCntxt->VSSetSamplers( s, min(iNr-s, MAX_CLEAR), sampler_clear);	
		iNr = m_sVertShader.sResourceViewList.iAllocatedArraySize;
		for(int r=0; r<iNr; r+= MAX_CLEAR) pd3dImmCntxt->VSSetShaderResources( r, min(iNr-r, MAX_CLEAR), srv_clear);
	}

	if(m_sHullShader.pShader!=NULL)	// Hull shader
	{
		iNr = m_sHullShader.sCnstBufferList.iAllocatedArraySize;
		for(int b=0; b<iNr; b+= MAX_CLEAR) pd3dImmCntxt->HSSetConstantBuffers( b, min(iNr-b, MAX_CLEAR), buf_clear);	
		iNr = m_sHullShader.sSamplerList.iAllocatedArraySize;
		for(int s=0; s<iNr; s+= MAX_CLEAR) pd3dImmCntxt->HSSetSamplers( s, min(iNr-s, MAX_CLEAR), sampler_clear);	
		iNr = m_sHullShader.sResourceViewList.iAllocatedArraySize;
		for(int r=0; r<iNr; r+= MAX_CLEAR) pd3dImmCntxt->HSSetShaderResources( r, min(iNr-r, MAX_CLEAR), srv_clear);
	}

	if(m_sDomainShader.pShader!=NULL)	// Domain shader
	{
		iNr = m_sDomainShader.sCnstBufferList.iAllocatedArraySize;
		for(int b=0; b<iNr; b+= MAX_CLEAR) pd3dImmCntxt->DSSetConstantBuffers( b, min(iNr-b, MAX_CLEAR), buf_clear);
		iNr = m_sDomainShader.sSamplerList.iAllocatedArraySize;
		for(int s=0; s<iNr; s+= MAX_CLEAR) pd3dImmCntxt->DSSetSamplers( s, min(iNr-s, MAX_CLEAR), sampler_clear);	
		iNr = m_sDomainShader.sResourceViewList.iAllocatedArraySize;
		for(int r=0; r<iNr; r+= MAX_CLEAR) pd3dImmCntxt->DSSetShaderResources( r, min(iNr-r, MAX_CLEAR), srv_clear);
	}

	if(m_sGeometryShader.pShader!=NULL)	// Geometry shader
	{
		iNr = m_sGeometryShader.sCnstBufferList.iAllocatedArraySize;
		for(int b=0; b<iNr; b+= MAX_CLEAR) pd3dImmCntxt->GSSetConstantBuffers( b, min(iNr-b, MAX_CLEAR), buf_clear);
		iNr = m_sGeometryShader.sSamplerList.iAllocatedArraySize;
		for(int s=0; s<iNr; s+= MAX_CLEAR) pd3dImmCntxt->GSSetSamplers( s, min(iNr-s, MAX_CLEAR), sampler_clear);	
		iNr = m_sGeometryShader.sResourceViewList.iAllocatedArraySize;
		for(int r=0; r<iNr; r+= MAX_CLEAR) pd3dImmCntxt->GSSetShaderResources( r, min(iNr-r, MAX_CLEAR), srv_clear);
	}

	if(m_sPixelShader.pShader!=NULL)	// Pixel shader
	{
		iNr = m_sPixelShader.sCnstBufferList.iAllocatedArraySize;
		for(int b=0; b<iNr; b+= MAX_CLEAR) pd3dImmCntxt->PSSetConstantBuffers( b, min(iNr-b, MAX_CLEAR), buf_clear);
		iNr = m_sPixelShader.sSamplerList.iAllocatedArraySize;
		for(int s=0; s<iNr; s+= MAX_CLEAR) pd3dImmCntxt->PSSetSamplers( s, min(iNr-s, MAX_CLEAR), sampler_clear);	
		iNr = m_sPixelShader.sResourceViewList.iAllocatedArraySize;
		for(int r=0; r<iNr; r+= MAX_CLEAR) pd3dImmCntxt->PSSetShaderResources( r, min(iNr-r, MAX_CLEAR), srv_clear);
	}
}

// register shaders before resources
void CShaderPipeline::SetVertexShader(CShader * pShader)
{
	if(m_bResourceBuffersUnregistered)
	{
		m_sVertShader.pShader = pShader;
		m_bVertShaderUnregistered = false;
	}
}

void CShaderPipeline::SetHullShader(CShader * pShader)
{
	if(m_bResourceBuffersUnregistered)
		m_sHullShader.pShader = pShader;
}

void CShaderPipeline::SetDomainShader(CShader * pShader)
{
	if(m_bResourceBuffersUnregistered)
		m_sDomainShader.pShader = pShader;
}

void CShaderPipeline::SetGeometryShader(CShader * pShader)
{
	if(m_bResourceBuffersUnregistered)
		m_sGeometryShader.pShader = pShader;
}

void CShaderPipeline::SetPixelShader(CShader * pShader)
{
	if(m_bResourceBuffersUnregistered)
		m_sPixelShader.pShader = pShader;
}

// register resources last
void CShaderPipeline::RegisterConstBuffer(const char cbName[], ID3D11Buffer * pBufferHandler)
{
	// sanity check
	assert(!m_bVertShaderUnregistered);
	if(!m_bVertShaderUnregistered)
	{
		ResourcePointer resource;
		resource.pCnstBuffer = pBufferHandler;

		m_bResourceBuffersUnregistered = false;
		RegResourceWithShader(m_sVertShader.pShader, &m_sVertShader.sCnstBufferList, cbName, resource);
		RegResourceWithShader(m_sHullShader.pShader, &m_sHullShader.sCnstBufferList, cbName, resource);
		RegResourceWithShader(m_sDomainShader.pShader, &m_sDomainShader.sCnstBufferList, cbName, resource);
		RegResourceWithShader(m_sGeometryShader.pShader, &m_sGeometryShader.sCnstBufferList, cbName, resource);
		RegResourceWithShader(m_sPixelShader.pShader, &m_sPixelShader.sCnstBufferList, cbName, resource);
	}
}

void CShaderPipeline::RegisterSampler(const char cbName[], ID3D11SamplerState * pSamplerHandler)
{
	// sanity check
	assert(!m_bVertShaderUnregistered);
	if(!m_bVertShaderUnregistered)
	{
		ResourcePointer resource;
		resource.pSamplerState = pSamplerHandler;

		m_bResourceBuffersUnregistered = false;
		RegResourceWithShader(m_sVertShader.pShader, &m_sVertShader.sSamplerList, cbName, resource);
		RegResourceWithShader(m_sHullShader.pShader, &m_sHullShader.sSamplerList, cbName, resource);
		RegResourceWithShader(m_sDomainShader.pShader, &m_sDomainShader.sSamplerList, cbName, resource);
		RegResourceWithShader(m_sGeometryShader.pShader, &m_sGeometryShader.sSamplerList, cbName, resource);
		RegResourceWithShader(m_sPixelShader.pShader, &m_sPixelShader.sSamplerList, cbName, resource);
	}
}

void CShaderPipeline::RegisterResourceView(const char cbName[], ID3D11ShaderResourceView * pResViewHandler)
{
	// sanity check
	assert(!m_bVertShaderUnregistered);
	if(!m_bVertShaderUnregistered)
	{
		ResourcePointer resource;
		resource.pResourceView = pResViewHandler;

		m_bResourceBuffersUnregistered = false;
		RegResourceWithShader(m_sVertShader.pShader, &m_sVertShader.sResourceViewList, cbName, resource);
		RegResourceWithShader(m_sHullShader.pShader, &m_sHullShader.sResourceViewList, cbName, resource);
		RegResourceWithShader(m_sDomainShader.pShader, &m_sDomainShader.sResourceViewList, cbName, resource);
		RegResourceWithShader(m_sGeometryShader.pShader, &m_sGeometryShader.sResourceViewList, cbName, resource);
		RegResourceWithShader(m_sPixelShader.pShader, &m_sPixelShader.sResourceViewList, cbName, resource);
	}
}

// 

void CShaderPipeline::RegResourceWithShader(CShader * pShader, SResourceList * pResourceList, const char cbName[], ResourcePointer sResourceHandler)
{
	if(pShader!=NULL)	// shader activated
	{
		assert(sResourceHandler.pVoidPtr!=NULL);
		assert(pResourceList->iAllocatedArraySize>=0);
		const int iSlot = pShader->GetResourceBufferSlot(cbName);

		// check if buffer is used at the current shader stage
		if(iSlot>=0)
		{
			// check if we need more memory
			const int iAllocatedArraySize = pResourceList->iAllocatedArraySize;
			if(iSlot>=iAllocatedArraySize)
			{
				const int iNewArraySize = iSlot+1;
				ResourcePointer * pResourceBuffer = new ResourcePointer[iNewArraySize];
				if(pResourceBuffer!=NULL)
				{
					// copy & delete old ones
					if(pResourceList->m_pList!=NULL)
					{
						for(int i=0; i<iAllocatedArraySize; i++)
							pResourceBuffer[i] = pResourceList->m_pList[i];

						// delete the old buffer
						delete [] pResourceList->m_pList;
						pResourceList->m_pList = NULL;
					}

					// clear new entries
					for(int i=iAllocatedArraySize; i<iNewArraySize; i++)
						pResourceBuffer[i].pVoidPtr=NULL;

					// update buffer
					pResourceList->iAllocatedArraySize = iNewArraySize;
					pResourceList->m_pList = pResourceBuffer;
				}
			}

			// insert into the correct slot
			pResourceList->m_pList[iSlot] = sResourceHandler;
		}
	}
}


CShaderPipeline::CShaderPipeline()
{
	m_bVertShaderUnregistered=true;
	m_bResourceBuffersUnregistered=true;

	assert(sizeof(ID3D11Buffer *)==sizeof(void *));
	assert(sizeof(ID3D11SamplerState *)==sizeof(void *));
	assert(sizeof(ID3D11ShaderResourceView *)==sizeof(void *));
}

CShaderPipeline::~CShaderPipeline()
{


}