#ifndef RENDERER_H
#define RENDERER_H

#pragma warning (push)
#pragma warning (disable: 4005)

#include <d3d9.h>
#include <d3d11.h>
#include <d3dx9.h>
#include <d3dx10.h>
#include <d3dx11.h>
#include <D3DX11tex.h>
#include <DXGI.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#pragma warning (pop)

#include <stdio.h>
#include <Windows.h>

#include "Helper.h"
#include "StateSaver.h"
#include "Shader.h"

class D3D11Renderer
{
private:
	struct VERTEX
	{
		FLOAT X, Y, Z;
		D3DXCOLOR Color;
	};
	
	IDXGISwapChain *pSwapChain;
	ID3D11Device *pDevice;
	ID3D11DeviceContext *pDeviceContext;
	ID3D11InputLayout *pInputLayout;
	ID3D11Buffer *pVertexBuffer;
	ID3D11VertexShader *pVertexShader;
	ID3D11PixelShader *pPixelShader;
	ID3D11BlendState *pTransparency;
	D3D11StateSaver* pStateSaver;
	
	bool restoreState = false;
	
public:
	D3D11Renderer(IDXGISwapChain *SwapChain);
	~D3D11Renderer();

	bool  InitD3D();
	bool  CompileShader(const char* szShader, const char* szEntrypoint, const char* szTarget, ID3D10Blob** pBlob);
	void  DrawFilledRect(const float x, const float y, const float w, const float h, D3DXCOLOR color);
	void  DrawLine(const float x1, const float y1, const float x2, const float y2, D3DXCOLOR color);
	void  DrawCircle(const float x, const float y, float radius, D3DXCOLOR color);
	void  DrawCircleThicc(const float x0, const float y0, float radius, const float thickness, D3DXCOLOR color);
	void  DrawHealthBar(const float x, const float y, const float w, const float h, float health, const float max);
	void  BeginScene();
	void  EndScene();
};

#endif