#pragma once

inline DXGI_SWAP_CHAIN_DESC DXGISwapChainDesc(HWND hWnd, UINT x, UINT y,
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT bufferCount = 1,
	DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL,
	DXGI_USAGE usage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
	DXGI_SCALING scaling = DXGI_SCALING_STRETCH)
{
	DXGI_SWAP_CHAIN_DESC result = { };
	result.BufferDesc.Width = x;
	result.BufferDesc.Height = y;
	result.BufferDesc.Format = format;
	result.SampleDesc.Count = 1;
	result.SampleDesc.Quality = 0;
	result.BufferUsage = usage;
	result.BufferCount = bufferCount;
	result.OutputWindow = hWnd;
	result.Windowed = true;
	result.SwapEffect = swapEffect;
	result.Flags = 0;

	return result;
}