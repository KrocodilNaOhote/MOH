#include "Renderer.h"

D3D11Renderer::D3D11Renderer(IDXGISwapChain* SwapChain)
{
	this->pDevice			= NULL;
	this->pDeviceContext	= NULL;
	this->pVertexShader		= NULL;
	this->pPixelShader		= NULL;
	this->pTransparency		= NULL;
	this->pInputLayout		= NULL;
	this->pVertexBuffer		= NULL;

	this->pSwapChain = SwapChain;

	this->pStateSaver = new D3D11StateSaver();
}

D3D11Renderer::~D3D11Renderer()
{
	SAFE_DELETE (this->pStateSaver);
	SAFE_RELEASE(this->pVertexShader);
	SAFE_RELEASE(this->pPixelShader);
	SAFE_RELEASE(this->pTransparency);
	SAFE_RELEASE(this->pInputLayout);
	SAFE_RELEASE(this->pVertexBuffer);
	SAFE_RELEASE(this->pSwapChain);
	SAFE_RELEASE(this->pDevice);
	SAFE_RELEASE(this->pDeviceContext);
}

bool D3D11Renderer::InitD3D()
{
	HRESULT hr;

	if (!this->pSwapChain)
		return false;

	this->pSwapChain->GetDevice(__uuidof(this->pDevice), (void**)&this->pDevice);
	if (!this->pDevice)
		return false;

	this->pDevice->GetImmediateContext(&this->pDeviceContext);
	if (!this->pDeviceContext)
		return false;

	ID3D10Blob* VS, * PS;

	if (!CompileShader(D3D11FillShader, "VS", "vs_4_0", &VS))
		return false;

	hr = this->pDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &this->pVertexShader);
	if (FAILED(hr))
	{
		SAFE_RELEASE(VS);
		return false;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	hr = this->pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), VS->GetBufferPointer(), VS->GetBufferSize(), &this->pInputLayout);
	SAFE_RELEASE(VS);
	if (FAILED(hr))
		return false;

	if (!CompileShader(D3D11FillShader, "PS", "ps_4_0", &PS))
		return false;

	hr = this->pDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &this->pPixelShader);
	if (FAILED(hr))
	{
		SAFE_RELEASE(PS);
		return false;
	}

	D3D11_BUFFER_DESC bufferDesc;

	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = 4 * sizeof(VERTEX);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	hr = this->pDevice->CreateBuffer(&bufferDesc, NULL, &this->pVertexBuffer);
	if (FAILED(hr))
		return false;

	D3D11_BLEND_DESC blendStateDescription;
	ZeroMemory(&blendStateDescription, sizeof(blendStateDescription));

	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	hr = this->pDevice->CreateBlendState(&blendStateDescription, &this->pTransparency);
	if (FAILED(hr))
		return false;

	return true;
}

bool D3D11Renderer::CompileShader(const char* cShader, const char* cEntrypoint, const char* cTarget, ID3D10Blob** pBlob)
{
	ID3D10Blob* pErrorBlob = nullptr;

	auto hr = D3DCompile(cShader, strlen(cShader), 0, nullptr, nullptr, cEntrypoint, cTarget, D3DCOMPILE_ENABLE_STRICTNESS, 0, pBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			char szError[256]{ 0 };
			memcpy(szError, pErrorBlob->GetBufferPointer(), pErrorBlob->GetBufferSize());
			MessageBoxA(nullptr, szError, "Error", MB_OK);
		}
		return false;
	}
	return true;
}

void D3D11Renderer::DrawFilledRect(const float x, const float y, const float w, const float h, D3DXCOLOR color)
{
	if (this->pDeviceContext == NULL)
		return;

	UINT viewportNumber = 1;
	D3D11_VIEWPORT vp;

	this->pDeviceContext->RSGetViewports(&viewportNumber, &vp);

	float x0 = x;
	float y0 = y;
	float x1 = x + w;
	float y1 = y + h;

	float xx0 = 2.0f * (x0 - 0.5f) / vp.Width - 1.0f;
	float yy0 = 1.0f - 2.0f * (y0 - 0.5f) / vp.Height;
	float xx1 = 2.0f * (x1 - 0.5f) / vp.Width - 1.0f;
	float yy1 = 1.0f - 2.0f * (y1 - 0.5f) / vp.Height;

	VERTEX* v = NULL;
	D3D11_MAPPED_SUBRESOURCE mapData;

	if (FAILED(this->pDeviceContext->Map(this->pVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mapData)))
		return;

	v = (VERTEX*)mapData.pData;

	v[0].X = (float)x0;
	v[0].Y = (float)y0;
	v[0].Z = 0.0f;
	v[0].Color = color;

	v[1].X = (float)x1;
	v[1].Y = (float)y1;
	v[1].Z = 0.0f;
	v[1].Color = color;

	v[0].X = xx0;
	v[0].Y = yy0;
	v[0].Z = 0.0f;
	v[0].Color = color;

	v[1].X = xx1;
	v[1].Y = yy0;
	v[1].Z = 0.0f;
	v[1].Color = color;

	v[2].X = xx0;
	v[2].Y = yy1;
	v[2].Z = 0.0f;
	v[2].Color = color;

	v[3].X = xx1;
	v[3].Y = yy1;
	v[3].Z = 0.0f;
	v[3].Color = color;


	this->pDeviceContext->Unmap(this->pVertexBuffer, NULL);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	this->pDeviceContext->OMSetBlendState(this->pTransparency, blendFactor, 0xffffffff);

	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;

	this->pDeviceContext->IASetVertexBuffers(0, 1, &this->pVertexBuffer, &Stride, &Offset);
	this->pDeviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	this->pDeviceContext->IASetInputLayout(this->pInputLayout);
		 
	this->pDeviceContext->VSSetShader(this->pVertexShader, 0, 0);
	this->pDeviceContext->PSSetShader(this->pPixelShader, 0, 0);
	this->pDeviceContext->GSSetShader(NULL, 0, 0);
	this->pDeviceContext->Draw(4, 0);
}

void D3D11Renderer::DrawLine(const float x1, const float y1, const float x2, const float y2, D3DXCOLOR color)
{
	if (this->pDeviceContext == NULL)
		return;

	UINT viewportNumber = 1;
	D3D11_VIEWPORT vp;

	this->pDeviceContext->RSGetViewports(&viewportNumber, &vp);

	VERTEX LineVertices[] =
	{
		{2.0f * (x1 - 0.5f) / vp.Width - 1.0f, 1.0f - 2.0f * (y1 - 0.5f) / vp.Height, 0.0f, color},
		{2.0f * (x2 - 0.5f) / vp.Width - 1.0f, 1.0f - 2.0f * (y2 - 0.5f) / vp.Height, 0.0f, color}
	};

	D3D11_MAPPED_SUBRESOURCE mapData;

	if (FAILED(this->pDeviceContext->Map(this->pVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mapData)))
		return;

	memcpy(mapData.pData, LineVertices, sizeof(LineVertices));

	this->pDeviceContext->Unmap(this->pVertexBuffer, NULL);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	this->pDeviceContext->OMSetBlendState(this->pTransparency, blendFactor, 0xffffffff);

	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;

	this->pDeviceContext->IASetVertexBuffers(0, 1, &this->pVertexBuffer, &Stride, &Offset);
	this->pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	this->pDeviceContext->IASetInputLayout(this->pInputLayout);
		
	this->pDeviceContext->VSSetShader(this->pVertexShader, 0, 0);
	this->pDeviceContext->PSSetShader(this->pPixelShader, 0, 0);
	this->pDeviceContext->GSSetShader(NULL, 0, 0);
	this->pDeviceContext->Draw(2, 0);
}

void D3D11Renderer::DrawCircle(const float x, const float y, float radius, D3DXCOLOR color)
{
	float fRadius = radius;
	D3DXCOLOR colorBlack = { 0.0f, 0.0f, 0.0f, 1.0f };

	if (fRadius <= 0)
		fRadius = 2;

	for (float i = 0.0; i < 40; i++)
	{
		float pointX0 = (x - 1.0) + fRadius * cos(i);
		float pointY0 = (y - 1.0) + fRadius * sin(i);
		float pointX1 = fRadius * cos(i + 0.3) + (x - 1.0);
		float pointY1 = fRadius * sin(i + 0.3) + (y - 1.0);
		DrawLine(pointX0, pointY0, pointX1, pointY1, colorBlack);
	}

	for (float i = 0.0; i < 40; i++)
	{
		float pointX0 = x + fRadius * cos(i);
		float pointY0 = y + fRadius * sin(i);
		float pointX1 = fRadius * cos(i + 0.3) + x;
		float pointY1 = fRadius * sin(i + 0.3) + y;
		DrawLine(pointX0, pointY0, pointX1, pointY1, color);
	}
}

void D3D11Renderer::DrawCircleThicc(const float x0, const float y0, float radius, const float thickness, D3DXCOLOR color)
{
	float fRadius = radius;

	if (fRadius <= 0)
		fRadius = 2;

	int x = fRadius, y = 0;
	int radiusError = 1 - x;

	while (x >= y)
	{
		this->DrawFilledRect(x + x0, y + y0, thickness, thickness, color);
		this->DrawFilledRect(y + x0, x + y0, thickness, thickness, color);
		this->DrawFilledRect(-x + x0, y + y0, thickness, thickness, color);
		this->DrawFilledRect(-y + x0, x + y0, thickness, thickness, color);
		this->DrawFilledRect(-x + x0, -y + y0, thickness, thickness, color);
		this->DrawFilledRect(-y + x0, -x + y0, thickness, thickness, color);
		this->DrawFilledRect(x + x0, -y + y0, thickness, thickness, color);
		this->DrawFilledRect(y + x0, -x + y0, thickness, thickness, color);
		y++;

		if (radiusError < 0)
		{
			radiusError += 2 * y + 1;
		}
		else 
		{
			x--;
			radiusError += 2 * (y - x + 1);
		}
	}
}

void D3D11Renderer::DrawHealthBar(const float x, const float y, const float w, const float h, float health, const float max)
{
	if (!max)
		return;

	if (w < 5)
		return;

	if (health < 0)
		health = 0;

	float ratio = health / max;

	D3DXCOLOR color = { (1.0f - 1.0f * ratio), (1.0f * ratio), 0.0f, 1.0f };
	D3DXCOLOR colorBlack = { 0.0f, 0.0f, 0.0f, 1.0f };

	float step = (w / max);
	float draw = (step * health);

	DrawFilledRect(x, y, w, h, colorBlack);
	DrawFilledRect(x, y, draw, h, color);
}

void D3D11Renderer::BeginScene()
{
	this->restoreState = false;
	if (SUCCEEDED(this->pStateSaver->saveCurrentState(this->pDeviceContext)))
		this->restoreState = true;

	this->pDeviceContext->IASetInputLayout(this->pInputLayout);
}

void D3D11Renderer::EndScene()
{
	if (this->restoreState)
		this->pStateSaver->restoreSavedState();
}