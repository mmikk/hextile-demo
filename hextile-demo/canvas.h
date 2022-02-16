#ifndef __CANVAS_H__
#define __CANVAS_H__

#include "texture_rt.h"
#include "shader.h"
#include "shaderpipeline.h"

class CCanvas
{
public:
	void InitCanvas(ID3D11Device* pd3dDevice, ID3D11Buffer * pGlobalsCB);
	void DrawCanvas(ID3D11DeviceContext* pd3dImmediateContext, ID3D11Buffer * pGlobalsCB);
	void CleanUp();
	
	CCanvas();
	~CCanvas();

private:

   
	CShader m_vert_shader;
	CShader m_pix_shader;
	CShaderPipeline m_ShaderPipeline;

	ID3D11DepthStencilState * m_pDepthStencilStateEqual_NoDepthWrite;

};

#endif