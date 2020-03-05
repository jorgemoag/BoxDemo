#include "DemoApp.h"

DemoApp::DemoApp()
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