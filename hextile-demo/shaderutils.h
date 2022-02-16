#ifndef __SHADERUTILS_H__
#define __SHADERUTILS_H__

#include <d3d11_2.h> 


ID3D11RasterizerState * GetDefaultRasterSolid();
ID3D11RasterizerState * GetDefaultRasterWire();
ID3D11RasterizerState * GetDefaultRasterSolidCullFront();
ID3D11RasterizerState * GetDefaultRasterSolidCullBack();
ID3D11SamplerState * GetDefaultShadowSampler();
ID3D11SamplerState * GetDefaultSamplerWrap();
ID3D11SamplerState * GetDefaultSamplerClamp();
ID3D11DepthStencilState * GetDefaultDepthStencilState();
ID3D11DepthStencilState * GetDefaultDepthStencilState_NoDepthWrite();


bool InitUtils(ID3D11Device* pd3dDevice);
void DeinitUtils();

unsigned int GetDXGIBitWidth(DXGI_FORMAT format);

#endif