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
	DemoApp();

private:
	ComPtr<IDXGIFactory4> Factory;
	ComPtr<ID3D12Device> Device;
};