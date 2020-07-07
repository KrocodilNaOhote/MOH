#include <Windows.h>
#include <iostream>
#include <d3d11.h>	
#include <d3dx11.h>	
#include <d3dx10.h> 
#include <DirectXMath.h>+

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3dx10.lib")

#include "VMTIndexes.h"
#include "Helper.h"
#include "Renderer.h"

// Globals
constexpr const int PRESENT_STUB_SIZE = 5;

ID3D11Device* pDevice = nullptr;
IDXGISwapChain* pSwapChain = nullptr;

using fnPresent = HRESULT(__stdcall*)(IDXGISwapChain * pThis, UINT SyncInterval, UINT Flags);
fnPresent ogPresentTramp;

void* ogPresent;
void* pTrampoline = nullptr;
char ogBytes[PRESENT_STUB_SIZE];

// Colors
D3DXCOLOR colorWhite	{ 1.0f, 1.0f, 1.0f, 1.0f };
D3DXCOLOR colorBlack	{ 0.0f, 0.0f, 0.0f, 1.0f };
D3DXCOLOR colorRed		{ 1.0f, 0.0f, 0.0f, 1.0f };
D3DXCOLOR colorGreen	{ 0.0f, 1.0f, 0.0f, 1.0f };
D3DXCOLOR colorBlue		{ 0.0f, 0.0f, 1.0f, 1.0f };

// Functions rototypes
bool Hook(void* pSrc, void* pDst, size_t size);
bool ReadMem(void* pDst, char* pBytes, size_t size);
bool HookD3D();
void CleanupD3D();

// Cheat logic
HRESULT __stdcall hkPresent(IDXGISwapChain* pThis, UINT SyncInterval, UINT Flags)
{
	std::cout << "[+] hkPresent() begin" << std::endl;
	pSwapChain = pThis;
	D3D11Renderer* renderer = new D3D11Renderer(pThis);

	if (!pDevice)
	{
		if (!renderer->InitD3D())
			return false;
	}

	std::cout << "[+] BeginScene() begin" << std::endl;
	renderer->BeginScene();
	// Draw here
	/*renderer->DrawFilledRect(50.0f, 50.0f, 100.0f, 20.0f, colorWhite);
	renderer->DrawLine(51.0f, 51.0f, 148.0f, 51.0f, colorRed);
	renderer->DrawLine(51.0f, 51.0f, 51.0f, 68.0f, colorRed);
	renderer->DrawLine(51.0f, 68.0f, 148.0f, 68.0f, colorRed);
	renderer->DrawLine(148.0f, 51.0f, 148.0f, 68.0f, colorRed);*/

	renderer->DrawCircle(500.0f, 500.0f, 10.0f, colorRed);

	renderer->DrawCircleThicc(300.0f, 300.0f, 0.0f, 2.0f, colorRed);

	std::cout << "[+] EndScene() begin" << std::endl;
	renderer->EndScene();

	return ogPresentTramp(pThis, SyncInterval, Flags);
}

// Main
void MainThread(void* pHandle)
{
	AllocConsole();
	FILE* console;
	freopen_s(&console, "CONOUT$", "w", stdout);

	if (HookD3D())
	{
		while (!GetAsyncKeyState(VK_END));
	}

	fclose(console);
	FreeConsole();
	CleanupD3D();
	ReadMem(ogPresent, ogBytes, PRESENT_STUB_SIZE);
	VirtualFree((void*)ogPresentTramp, 0x1000, MEM_RELEASE);
	CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)FreeLibrary, pHandle, 0, nullptr);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hinstDLL, 0, nullptr);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

bool Hook(void* pSrc, void* pDst, size_t size)
{
	std::cout << "[+] Hook() begin" << std::endl;
	DWORD dwOldProtect = 0;
	uintptr_t src = (uintptr_t)pSrc;
	uintptr_t dst = (uintptr_t)pDst;

	if (!VirtualProtect(pSrc, size, PAGE_EXECUTE_READWRITE, &dwOldProtect))
		return false;

	*(char*)src = (char)0XE9;
	*(int*)(src + 1) = (int)(dst - src - 5);

	VirtualProtect(pSrc, size, dwOldProtect, &dwOldProtect);
	return true;
}

bool ReadMem(void* pDst, char* pBytes, size_t size)
{
	std::cout << "[+] ReadMem() begin" << std::endl;
	DWORD dwOldProtect = 0;
	if (!VirtualProtect(pDst, size, PAGE_EXECUTE_READWRITE, &dwOldProtect))
		return false;

	memcpy(pDst, pBytes, PRESENT_STUB_SIZE);

	VirtualProtect(pDst, size, dwOldProtect, &dwOldProtect);

	return true;
}

bool HookD3D()
{
	std::cout << "[+] HookD3D() begin" << std::endl;
	D3D_FEATURE_LEVEL featLevel;
	DXGI_SWAP_CHAIN_DESC sd{ 0 };
	sd.BufferCount = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.Height = 1920;
	sd.BufferDesc.Width = 1080;
	sd.BufferDesc.RefreshRate = { 60, 1 };
	sd.OutputWindow = GetForegroundWindow();
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_REFERENCE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &pSwapChain, &pDevice, &featLevel, nullptr);
	if (FAILED(hr))
		return false;

	void** pVMT = *(void***)pSwapChain;
	ogPresent = (fnPresent)(pVMT[(UINT)IDXGISwapChainVMT::Present]);

	SAFE_RELEASE(pSwapChain);
	SAFE_RELEASE(pDevice);

	void* pLoc = (void*)((uintptr_t)ogPresent - 0x2000);
	void* pTrampLoc = nullptr;
	while (!pTrampLoc)
	{
		pTrampLoc = VirtualAlloc(pLoc, 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		pLoc = (void*)((uintptr_t)pLoc + 0x200);
		std::cout << "[+] Trying to find pTrampLoc" << std::endl;
	}
	std::cout << "[+] pTrampLoc was found" << std::endl;
	ogPresentTramp = (fnPresent)pTrampLoc;

	memcpy(ogBytes, ogPresent, PRESENT_STUB_SIZE);
	memcpy(pTrampLoc, ogBytes, PRESENT_STUB_SIZE);

	pTrampLoc = (void*)((uintptr_t)pTrampLoc + PRESENT_STUB_SIZE);

	*(char*)pTrampLoc = (char)0xE9;
	pTrampLoc = (void*)((uintptr_t)pTrampLoc + 1);
	uintptr_t ogPresRet = (uintptr_t)ogPresent + 5;
	*(int*)pTrampLoc = ogPresRet - (int)(uintptr_t)pTrampLoc - 4;

	pTrampoline = pTrampLoc = (void*)((uintptr_t)pTrampLoc + 4);

	char pJmp[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
	std::cout << "[+] ReadMem() calling" << std::endl;
	ReadMem(pTrampLoc, pJmp, ARRAYSIZE(pJmp));
	pTrampLoc = (void*)((uintptr_t)pTrampLoc + ARRAYSIZE(pJmp));
	std::cout << "[+] hkPresent() calling" << std::endl;
	*(uintptr_t*)pTrampLoc = (uintptr_t)hkPresent;

	std::cout << "[+] Hook() calling" << std::endl;
	return Hook(ogPresent, pTrampoline, PRESENT_STUB_SIZE);
}

void CleanupD3D()
{
	SAFE_RELEASE(pSwapChain);
	SAFE_RELEASE(pDevice);
}