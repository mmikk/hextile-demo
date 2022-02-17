#ifndef __SCENEGRAPH_H__
#define __SCENEGRAPH_H__

class ID3D11Device;
class ID3D11DeviceContext;
class ID3D11Buffer;
class ID3D11ShaderResourceView;
struct Vec3;

bool InitializeSceneGraph(ID3D11Device* pd3dDevice, ID3D11DeviceContext *pContext, ID3D11Buffer * pGlobalsCB);
void PassShadowResolve(ID3D11ShaderResourceView * pShadowResolveSRV);
void ReleaseSceneGraph();
void RenderSceneGraph(ID3D11DeviceContext *pContext, bool bSimpleLayout, bool bSkipGroundPlane=false);
Vec3 GetSunDir();


// shadow support functions
struct Vec3;
struct Mat44;

int GetNumberOfShadowCastingMeshInstances();
void GetAABBoxAndTransformOfShadowCastingMeshInstance(Vec3 * pvMin, Vec3 * pvMax, Mat44 * pmMat, const int idx_in);
void ToggleDetailTex(bool toggleIsForColor);

#endif