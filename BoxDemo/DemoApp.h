// DemoApp.h

#pragma once

// ComPtr<T>
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <vector>

using namespace DirectX;

static const UINT8 kFrameCount = 2;

class DemoApp
{
public:
	DemoApp(HWND hWnd, UINT Width, UINT Height);

	/* Run */
	void Tick();

private:
	/* Device */
	ComPtr<IDXGIFactory4> Factory;
	ComPtr<ID3D12Device> Device;

	void CreateDevice();

	/* Queue */
	ComPtr<ID3D12CommandQueue> CommandQueue;
	ComPtr<ID3D12CommandAllocator> CommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> CommandList;

	void CreateQueues();

	/* Fences */
	ComPtr<ID3D12Fence> Fence;
	HANDLE FenceEvent;
	UINT64 FenceValue;

	void CreateFence();
	void FlushAndWait();

	/* Pipeline */
	ComPtr<ID3DBlob> LoadShader(LPCWSTR Filename, LPCSTR EntryPoint, LPCSTR Target);
	ComPtr<ID3D12RootSignature> RootSignature;
	ComPtr<ID3D12PipelineState> PipelineState;
	void CreateRootSignature();
	void CreatePipeline();

	/* Swapchain */
	ComPtr<IDXGISwapChain3> Swapchain;
	ComPtr<ID3D12DescriptorHeap> RenderTargetViewHeap;
	ComPtr<ID3D12Resource> RenderTargets[kFrameCount]; // extracted from Swapchain
	void CreateRenderTargets();
	void CreateSwapchain(HWND hWnd, UINT Width, UINT Height);

	/* Record command list for render */
	void RecordCommandList();

	/* Vertex Buffers */
	struct Vertex
	{
		XMFLOAT3 Position;
		XMFLOAT2 UV;
		XMFLOAT4 Color;
	};
	ComPtr<ID3D12Resource> VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	void CreateVertexBuffer();

	/* Viewport and Scissor */
	D3D12_VIEWPORT Viewport;
	D3D12_RECT ScissorRect;
	void SetViewportAndScissorRect(int Width, int Height);


	/* Constant Buffer */
	ComPtr<ID3D12Resource> ConstantBufferResource;
	ComPtr<ID3D12DescriptorHeap> ContantBufferDescriptorHeap;
	UINT8* pConstantBufferData; // puntero que obtendremos con ConstantBufferResource->Map(...)
	void CreateConstantBuffer();

	/* Constant Buffer Values */
	struct FSomeConstants
	{
		XMMATRIX MVP;
	};
	FSomeConstants SomeConstants;
	void UpdateConstantBuffer();

	/* Resource */
	ComPtr<ID3D12Resource> TextureResource;
	ComPtr<ID3D12DescriptorHeap> TextureDescriptorHeap;
	std::vector<UINT8> GenerateTextureData();
	void LoadTexture();

};