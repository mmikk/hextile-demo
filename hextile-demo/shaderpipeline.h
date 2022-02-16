#ifndef __SHADERPIPELINE_H__
#define __SHADERPIPELINE_H__


#include <d3d11_2.h> 

class CShader;

class CShaderPipeline
{
public:
	// ready for render
	void PrepPipelineForRendering(ID3D11DeviceContext* pd3dImmCntxt);
	void FlushResources(ID3D11DeviceContext* pd3dImmCntxt);

	// register shaders before command buffers
	void SetVertexShader(CShader * pShader);
	void SetHullShader(CShader * pShader);
	void SetDomainShader(CShader * pShader);
	void SetGeometryShader(CShader * pShader);
	void SetPixelShader(CShader * pShader);

	// register constant buffers last
	void RegisterConstBuffer(const char cbName[], ID3D11Buffer * pBufferHandler);
	void RegisterSampler(const char cbName[], ID3D11SamplerState * pSamplerHandler);
	void RegisterResourceView(const char cbName[], ID3D11ShaderResourceView * pResViewHandler);

	CShaderPipeline();
	~CShaderPipeline();

private:
	// pipeline must have "at least" a vertex
	// shader to be complete.
	bool m_bVertShaderUnregistered;

	// did not yet receive any names of resources
	bool m_bResourceBuffersUnregistered;

	union ResourcePointer
	{
		ID3D11Buffer * pCnstBuffer;
		ID3D11SamplerState * pSamplerState;
		ID3D11ShaderResourceView * pResourceView;
		void * pVoidPtr;
	};

	struct SResourceList
	{
		SResourceList() : m_pList(NULL), iAllocatedArraySize(0) {}
		
		int iAllocatedArraySize;
		ResourcePointer * m_pList;
	};

	struct SShaderHeader
	{
		SShaderHeader() : pShader(NULL) {}
		CShader * pShader;
		
		SResourceList sCnstBufferList;
		SResourceList sSamplerList;
		SResourceList sResourceViewList;
	};

	SShaderHeader m_sVertShader;
	SShaderHeader m_sHullShader;
	SShaderHeader m_sDomainShader;
	SShaderHeader m_sGeometryShader;
	SShaderHeader m_sPixelShader;

	void RegResourceWithShader(CShader * pShader, SResourceList * pResourceList, const char cbName[], ResourcePointer sResourceHandler);
};

#endif