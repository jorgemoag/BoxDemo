// GPUMem.cpp
#include "GPUMem.h"

ComPtr<ID3D12Resource> GPUMem::Buffer(
	ID3D12Device* Device,
	SIZE_T SizeInBytes,
	D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_UPLOAD
)
{
	ComPtr<ID3D12Resource> Resource;

	D3D12_RESOURCE_DESC ResourceDesc{};
	ResourceDesc.Width = SizeInBytes;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;

	D3D12_HEAP_PROPERTIES HeapProps{};
	HeapProps.Type = HeapType;

	Device->CreateCommittedResource(
		&HeapProps,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Resource)
	);

	return Resource;
}

void GPUMem::ResourceBarrier(
	ID3D12GraphicsCommandList* CommandList,
	ID3D12Resource* Resource,
	D3D12_RESOURCE_STATES FromState,
	D3D12_RESOURCE_STATES ToState
)
{
	D3D12_RESOURCE_BARRIER ResourceBarrierDesc{};
	ResourceBarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrierDesc.Transition.pResource = Resource;
	ResourceBarrierDesc.Transition.StateBefore = FromState; // el estado previo
	ResourceBarrierDesc.Transition.StateAfter = ToState; // el estado posterior
	ResourceBarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	CommandList->ResourceBarrier(
		1, // cuantas resource barriers
		&ResourceBarrierDesc // array con las resources barriers
	);
}