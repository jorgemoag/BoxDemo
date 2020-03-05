#include "DemoApp.h"
#include "GPUMem.h"
#include "TextureLoader.h"
#include <vector>

DemoApp::DemoApp(HWND hWnd, UINT Width, UINT Height)
{
	CreateDevice();
	CreateQueues();
	CreateFence();

	CreateSwapchain(hWnd, Width, Height);
	CreateRenderTargets();

	CreateRootSignature();
	CreatePipeline();

	SetViewportAndScissorRect(Width, Height);
	CreateVertexBuffer();
	LoadTexture();

	CreateConstantBuffer();
}

void DemoApp::CreateDevice()
{
	ComPtr<ID3D12Debug> DebugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
	{
		DebugController->EnableDebugLayer();
	}

	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&Factory));

	ComPtr<IDXGIAdapter1> Adapter;
	bool bAdapterFound = false;

	for (UINT AdapterIndex = 0;
		!bAdapterFound && Factory->EnumAdapters1(AdapterIndex, &Adapter) != DXGI_ERROR_NOT_FOUND;
		++AdapterIndex)
	{
		DXGI_ADAPTER_DESC1 AdapterDesc;
		Adapter->GetDesc1(&AdapterDesc);

		if (AdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		HRESULT hr;
		hr = D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			bAdapterFound = true;
		}
	}

	D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));
}

void DemoApp::CreateQueues()
{
	// CommandQueue

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc{};

	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	CommandQueueDesc.NodeMask = 0;
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	Device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&CommandQueue));

	// Command Allocator
	Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator));

	// Command List
	ID3D12PipelineState* InitialState = nullptr;
	Device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		CommandAllocator.Get(),
		InitialState,
		IID_PPV_ARGS(&CommandList)
	);
	CommandList->Close();
}

void DemoApp::CreateFence()
{
	FenceValue = 0;

	Device->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));

	FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void DemoApp::FlushAndWait()
{
	const UINT64 FenceValueToSignal = FenceValue;
	CommandQueue->Signal(Fence.Get(), FenceValueToSignal);

	++FenceValue;

	if (Fence->GetCompletedValue() < FenceValueToSignal)
	{
		Fence->SetEventOnCompletion(FenceValueToSignal, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}

ComPtr<ID3DBlob> DemoApp::LoadShader(LPCWSTR Filename, LPCSTR EntryPoint, LPCSTR Target)
{
	HRESULT hr;

	/* Shaders */

	ComPtr<ID3DBlob> ShaderBlob;
	hr = D3DCompileFromFile(
		Filename, // FileName
		nullptr, nullptr, // MacroDefines, Includes, 		
		EntryPoint, // FunctionEntryPoint
		Target, // Target: "vs_5_0", "ps_5_0", "vs_5_1", "ps_5_1"
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // Compile flags
		0, // Flags2
		&ShaderBlob, // Code
		nullptr // Error
	);
	if (FAILED(hr))
	{
		OutputDebugString(L"[ERROR] D3DCompileFromFile -- Vertex shader");
	}

	return ShaderBlob;
}

std::vector<UINT8> DemoApp::GenerateTextureData()
{
	const int TextureWidth = 256;
	const int TextureHeight = 256;
	const int TexturePixelSize = 4; // The number of bytes used to represent a pixel in the texture.

	const UINT rowPitch = TextureWidth * TexturePixelSize;
	const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
	const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * TextureHeight;

	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	for (UINT n = 0; n < textureSize; n += TexturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0x00;        // R
			pData[n + 1] = 0x00;    // G
			pData[n + 2] = 0x00;    // B
			pData[n + 3] = 0xff;    // A
		}
		else
		{
			pData[n] = 0xff;        // R
			pData[n + 1] = 0xff;    // G
			pData[n + 2] = 0xff;    // B
			pData[n + 3] = 0xff;    // A
		}
	}

	return data;
}

void DemoApp::CreateRootSignature()
{
	// Dos parametros: Un constant buffer para MVP y un descriptor table a la textura
	D3D12_ROOT_PARAMETER RootParameters[2];

	// Constant Buffer
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	RootParameters[0].Descriptor.RegisterSpace = 0;
	RootParameters[0].Descriptor.ShaderRegister = 0;

	// Texture
	D3D12_DESCRIPTOR_RANGE TexDescriptorRanges[1];
	TexDescriptorRanges[0].BaseShaderRegister = 0;
	TexDescriptorRanges[0].NumDescriptors = 1;
	TexDescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;
	TexDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	TexDescriptorRanges[0].RegisterSpace = 0;

	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(TexDescriptorRanges);
	RootParameters[1].DescriptorTable.pDescriptorRanges = TexDescriptorRanges;

	// Samplers
	D3D12_STATIC_SAMPLER_DESC SamplerDesc{};

	SamplerDesc.ShaderRegister = 0;
	SamplerDesc.RegisterSpace = 0;
	SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	SamplerDesc.MipLODBias = 0;
	SamplerDesc.MaxAnisotropy = 0;
	SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	SamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	SamplerDesc.MinLOD = 0.0f;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	// Create Root Signature
	D3D12_ROOT_SIGNATURE_DESC SignatureDesc{};
	SignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	SignatureDesc.NumParameters = _countof(RootParameters);
	SignatureDesc.pParameters = RootParameters;
	SignatureDesc.NumStaticSamplers = 1;
	SignatureDesc.pStaticSamplers = &SamplerDesc;

	ComPtr<ID3DBlob> Signature;
	ComPtr<ID3DBlob> Error;

	D3D12SerializeRootSignature(&SignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &Signature, &Error);
	Device->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
}

void DemoApp::CreatePipeline()
{
	/* Shaders */

	ComPtr<ID3DBlob> VertexBlob = LoadShader(L"shaders.hlsl", "VSMain", "vs_5_1");

	D3D12_SHADER_BYTECODE VertexShaderBytecode;
	VertexShaderBytecode.pShaderBytecode = VertexBlob->GetBufferPointer();
	VertexShaderBytecode.BytecodeLength = VertexBlob->GetBufferSize();

	ComPtr<ID3DBlob> PixelBlob = LoadShader(L"shaders.hlsl", "PSMain", "ps_5_1");

	D3D12_SHADER_BYTECODE PixelShaderBytecode;
	PixelShaderBytecode.pShaderBytecode = PixelBlob->GetBufferPointer();
	PixelShaderBytecode.BytecodeLength = PixelBlob->GetBufferSize();

	/* Input Layout */
	// vamos a usar un Vertex con position, uv y color
	D3D12_INPUT_ELEMENT_DESC pInputElementDescs[] = {
		// SemanticName; SemanticIndex; Format; InputSlot; AlignedByteOffset; InputSlotClass; InstanceDataStepRate;
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * (3 + 2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC InputLayout{};
	InputLayout.NumElements = _countof(pInputElementDescs);
	InputLayout.pInputElementDescs = pInputElementDescs;

	/* Rasterizer Stage */

	D3D12_RASTERIZER_DESC RasterizerState{};
	RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	RasterizerState.FrontCounterClockwise = FALSE;
	RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterizerState.DepthClipEnable = TRUE;
	RasterizerState.MultisampleEnable = FALSE;
	RasterizerState.AntialiasedLineEnable = FALSE;
	RasterizerState.ForcedSampleCount = 0;
	RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	/* Blend State */

	D3D12_BLEND_DESC BlendState{};
	BlendState.AlphaToCoverageEnable = FALSE;
	BlendState.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC DefaultRenderTargetBlendDesc =
	{
	  FALSE,FALSE,
	  D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
	  D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
	  D3D12_LOGIC_OP_NOOP,
	  D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		BlendState.RenderTarget[i] = DefaultRenderTargetBlendDesc;
	}

	/* Depth Stencil */

	D3D12_DEPTH_STENCIL_DESC DepthStencilState{};
	DepthStencilState.DepthEnable = FALSE;
	DepthStencilState.StencilEnable = FALSE;

	/* Pipeline */

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc{};
	PSODesc.InputLayout = InputLayout;
	PSODesc.pRootSignature = RootSignature.Get();
	PSODesc.VS = VertexShaderBytecode;
	PSODesc.PS = PixelShaderBytecode;
	PSODesc.RasterizerState = RasterizerState;
	PSODesc.BlendState = BlendState;
	PSODesc.DepthStencilState = DepthStencilState;
	PSODesc.SampleMask = UINT_MAX;
	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSODesc.NumRenderTargets = 1;
	PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PSODesc.SampleDesc.Count = 1;

	Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PipelineState));
}

void DemoApp::CreateRenderTargets()
{
	/* RenderTargetHeap */
	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc{};
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = kFrameCount;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	Device->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&RenderTargetViewHeap));

	for (UINT FrameIndex = 0; FrameIndex < kFrameCount; ++FrameIndex)
	{
		Swapchain->GetBuffer(FrameIndex, IID_PPV_ARGS(&RenderTargets[FrameIndex]));

		D3D12_RENDER_TARGET_VIEW_DESC RTDesc{};
		RTDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		RTDesc.Texture2D.MipSlice = 0;
		RTDesc.Texture2D.PlaneSlice = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor = RenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
		DestDescriptor.ptr += ((SIZE_T)FrameIndex) * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		Device->CreateRenderTargetView(RenderTargets[FrameIndex].Get(), &RTDesc, DestDescriptor);
	}
}

void DemoApp::CreateSwapchain(HWND hWnd, UINT Width, UINT Height)
{
	/* Swapchain */
	DXGI_SWAP_CHAIN_DESC1 SwapchainDesc{};

	SwapchainDesc.BufferCount = 2;
	SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	SwapchainDesc.Width = Width;
	SwapchainDesc.Height = Height;
	SwapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	SwapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	SwapchainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> SwapchainTemp;
	Factory->CreateSwapChainForHwnd(
		CommandQueue.Get(), // swap chain forces flush when does flip
		hWnd,
		&SwapchainDesc,
		nullptr,
		nullptr,
		&SwapchainTemp
	);

	SwapchainTemp.As(&Swapchain);
}

void DemoApp::RecordCommandList()
{
	const UINT BackFrameIndex = Swapchain->GetCurrentBackBufferIndex();

	CommandAllocator->Reset();
	CommandList->Reset(CommandAllocator.Get(), PipelineState.Get());

	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetDescriptor = RenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	RenderTargetDescriptor.ptr += ((SIZE_T)BackFrameIndex) * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	GPUMem::ResourceBarrier(CommandList.Get(), RenderTargets[BackFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const FLOAT ClearValue[] = { 0.02f, 0.02f, 0.15f, 1.0f };
	CommandList->ClearRenderTargetView(RenderTargetDescriptor, ClearValue, 0, nullptr);
	CommandList->OMSetRenderTargets(1, &RenderTargetDescriptor, FALSE, nullptr);

	CommandList->SetGraphicsRootSignature(RootSignature.Get());
	CommandList->RSSetViewports(1, &Viewport);
	CommandList->RSSetScissorRects(1, &ScissorRect);

	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);

	CommandList->SetGraphicsRootConstantBufferView(0, ConstantBufferResource->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* Heaps[]{ TextureDescriptorHeap.Get() };
	CommandList->SetDescriptorHeaps(_countof(Heaps), Heaps);
	CommandList->SetGraphicsRootDescriptorTable(1, TextureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	CommandList->DrawInstanced(3, 1, 0, 0);

	GPUMem::ResourceBarrier(CommandList.Get(), RenderTargets[BackFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	CommandList->Close();
}

void DemoApp::CreateVertexBuffer()
{
	Vertex Vertices[] = {
		// { POS, UV, COLOR }
		{ { 0.0f, 0.5f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
	};

	VertexBuffer = GPUMem::Buffer(Device.Get(), sizeof(Vertices), D3D12_HEAP_TYPE_DEFAULT);

	ComPtr<ID3D12Resource> UploadBuffer = GPUMem::Buffer(Device.Get(), sizeof(Vertices), D3D12_HEAP_TYPE_UPLOAD);

	UINT8* pData;
	D3D12_RANGE ReadRange{ 0, 0 };
	UploadBuffer->Map(0, &ReadRange, reinterpret_cast<void**>(&pData));
	memcpy(pData, Vertices, sizeof(Vertices));
	UploadBuffer->Unmap(0, nullptr);

	CommandAllocator->Reset();
	CommandList->Reset(CommandAllocator.Get(), nullptr);

	GPUMem::ResourceBarrier(CommandList.Get(), VertexBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->CopyResource(VertexBuffer.Get(), UploadBuffer.Get());
	CommandList->Close();

	ID3D12CommandList* const pCommandList[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(1, pCommandList);

	VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.SizeInBytes = sizeof(Vertices);
	VertexBufferView.StrideInBytes = sizeof(Vertex);

	FlushAndWait();
}

void DemoApp::SetViewportAndScissorRect(int Width, int Height)
{
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = static_cast<FLOAT>(Width);
	Viewport.Height = static_cast<FLOAT>(Height);
	Viewport.MinDepth = D3D12_MIN_DEPTH;
	Viewport.MaxDepth = D3D12_MAX_DEPTH;

	ScissorRect.left = 0;
	ScissorRect.top = 0;
	ScissorRect.right = Width;
	ScissorRect.bottom = Height;
}

void DemoApp::CreateConstantBuffer()
{
	// Esta línea tiene un error que enseguida vamos a corregir -->
	UINT SizeInBytes = (sizeof(FSomeConstants) + 256) & 256;

	ConstantBufferResource = GPUMem::Buffer(Device.Get(), SizeInBytes, D3D12_HEAP_TYPE_UPLOAD);

	/* Descriptor Heap */
	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc{};
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // constant buffer

	Device->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&ContantBufferDescriptorHeap));

	/* Create descriptor */
	D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferViewDesc{};
	ConstantBufferViewDesc.BufferLocation = ConstantBufferResource->GetGPUVirtualAddress();
	ConstantBufferViewDesc.SizeInBytes = SizeInBytes;
	Device->CreateConstantBufferView(&ConstantBufferViewDesc, ContantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_RANGE ReadRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;
	ConstantBufferResource->Map(0, &ReadRange, reinterpret_cast<void**>(&pConstantBufferData));
}

void DemoApp::UpdateConstantBuffer()
{
	static float Angle = 0.0f;

	Angle += 0.005f;

	SomeConstants.MVP = XMMatrixRotationZ(Angle);

	memcpy(pConstantBufferData, &SomeConstants, sizeof(FSomeConstants));
}

void DemoApp::LoadTexture()
{
	TextureLoader Tex(L"texture.png");

	TextureResource = GPUMem::Texture2D(Device.Get(), Tex.GetWidth(), Tex.GetHeight(), Tex.GetFormat());

	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc{};
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	Device->CreateDescriptorHeap(&DescriptorHeapDesc, IID_PPV_ARGS(&TextureDescriptorHeap));

	D3D12_SHADER_RESOURCE_VIEW_DESC ResourceViewDesc{};
	ResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	ResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	ResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ResourceViewDesc.Texture2D.MipLevels = 1;
	Device->CreateShaderResourceView(TextureResource.Get(), &ResourceViewDesc, TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	SIZE_T SizeInBytes = (Tex.Size() + 256) & ~255;
	ComPtr<ID3D12Resource> UploadBuffer = GPUMem::Buffer(Device.Get(), SizeInBytes, D3D12_HEAP_TYPE_UPLOAD);

	UINT8* pData;
	UploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
	memcpy(pData, Tex.Pointer(), Tex.Size());
	UploadBuffer->Unmap(0, nullptr);

	CommandAllocator->Reset();
	CommandList->Reset(CommandAllocator.Get(), nullptr);

	D3D12_TEXTURE_COPY_LOCATION Dst{};
	Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	Dst.SubresourceIndex = 0;
	Dst.pResource = TextureResource.Get();

	D3D12_TEXTURE_COPY_LOCATION Src{};
	Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	Src.pResource = UploadBuffer.Get();
	Src.PlacedFootprint.Footprint.Depth = 1;
	Src.PlacedFootprint.Footprint.Format = Tex.GetFormat();
	Src.PlacedFootprint.Footprint.Width = Tex.GetWidth();
	Src.PlacedFootprint.Footprint.Height = Tex.GetHeight();
	Src.PlacedFootprint.Footprint.RowPitch = Tex.GetRowPitch();

	CommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

	GPUMem::ResourceBarrier(
		CommandList.Get(),
		TextureResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	CommandList->Close();

	ID3D12CommandList* CommandLists[]{ CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

	FlushAndWait();
}

void DemoApp::Tick()
{
	UpdateConstantBuffer();
	RecordCommandList();
	ID3D12CommandList* ppCommandLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	Swapchain->Present(1, 0);
	FlushAndWait();
}