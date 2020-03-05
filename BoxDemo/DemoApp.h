// DemoApp.h

#pragma once

// ComPtr<T>
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h> // D3D12xxxx
#include <dxgi1_6.h> // factories
#include <d3dcompiler.h> // compilar shaders
#include <DirectXMath.h> // matematicas

class DemoApp
{
public:
	static const UINT kFrameCount = 2;

	DemoApp(HWND hWnd, UINT Width, UINT Height);

	/* Run */
	void Tick();

private:
	ComPtr<IDXGIFactory4> Factory;
	ComPtr<ID3D12Device> Device;

	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> CommandList;

	void CreateDevice();
	void CreateQueues();

	/* Fences */
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;
	UINT64 FenceValue;

	void CreateFence();
	void FlushAndWait();

	/* Swapchain */
	ComPtr<IDXGISwapChain3> Swapchain;
	ComPtr<ID3D12DescriptorHeap> RenderTargetViewHeap;
	ComPtr<ID3D12Resource> RenderTargets[kFrameCount]; // extracted from Swapchain
	void CreateRenderTargets();
	void CreateSwapchain(HWND hWnd, UINT Width, UINT Height);

	/* Pipeline */
	ComPtr<ID3DBlob> LoadShader(LPCWSTR Filename, LPCSTR EntryPoint, LPCSTR Target);
	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12PipelineState> PipelineState;
	void CreateRootSignature();
	void CreatePipeline();

	/* Record command list for render */
	void RecordCommandList();
};