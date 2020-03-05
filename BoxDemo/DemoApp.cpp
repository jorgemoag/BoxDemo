#include "DemoApp.h"
#include "GPUMem.h"

DemoApp::DemoApp()
{
	CreateDevice();
	CreateQueues();
	CreateFence();
}

void DemoApp::CreateDevice()
{
	// Capa de depuracion
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
		hr = D3D12CreateDevice(Adapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			_uuidof(ID3D12Device),
			nullptr);
		if (SUCCEEDED(hr))
		{
			bAdapterFound = true;
		}
	}

	D3D12CreateDevice(Adapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&Device));
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

	// Por defecto cuando creas un CommandList está grabando  
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
	// encolamos a la GPU el comando:
	// "setea el valor de fence con este valor"
	const UINT64 FenceValueToSignal = FenceValue;
	CommandQueue->Signal(Fence.Get(), FenceValueToSignal);

	// incrementamos FenceValue 
	// para la siguiente vez que se llame
	++FenceValue;

	// si el valor de fence aún no es el valor que le dijimos
	// a la GPU que marcase, significa que aún no ha llegado
	// a ese comando. Esperamos.
	if (Fence->GetCompletedValue() < FenceValueToSignal)
	{
		Fence->SetEventOnCompletion(FenceValueToSignal, FenceEvent);
		WaitForSingleObject(FenceEvent, INFINITE);
	}
}