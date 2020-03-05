// GPUMem.h
#pragma once
#include <wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>

class GPUMem
{
public:

	static ComPtr<ID3D12Resource> Buffer(ID3D12Device* Device, SIZE_T SizeInBytes, D3D12_HEAP_TYPE HeapType);

	static void ResourceBarrier(ID3D12GraphicsCommandList* CommandList, ID3D12Resource* Resource, D3D12_RESOURCE_STATES FromState, D3D12_RESOURCE_STATES ToState);

};
