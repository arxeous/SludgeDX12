#pragma once
#include "pch.h"
namespace sludge::utils
{
	std::vector<uint8_t> LoadCSOFile(const std::wstring& filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			OutputDebugStringW((L"Failed to open: " + filename + L"\n").c_str());
			return {};  // Empty vector
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		if (fileSize == 0) {
			OutputDebugStringW((L"File is empty: " + filename + L"\n").c_str());
			return {};
		}

		std::vector<uint8_t> buffer(fileSize);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

		if (!file) {
			OutputDebugStringW((L"Failed to read: " + filename + L"\n").c_str());
			return {};
		}

		file.close();

		char dbg[256];
		sprintf_s(dbg, "Successfully read %zu bytes\n", fileSize);
		OutputDebugStringA(dbg);

		return buffer;
	}


	inline void TransitionResource(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
	{
		auto transitionBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, from, to);
		cmdList->ResourceBarrier(1, &transitionBarrier);
	}

	// Good ol span. ITs a lightweight abstraction for contiguous sequence of values of type T somewhere in memory. 
	// Its pretty much created for cases like these where we care about not only the buffer itself, but also its size! 
	// especially in contexts like function parameter for creating stuff from said buffer like below.
	template <typename T>
	inline std::pair<ID3D12Resource*, ID3D12Resource*> CreateGPUBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::span<const T> bufferData, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE)
	{
		ID3D12Resource* destinationResource{ nullptr };
		ID3D12Resource* intermediateResource{ nullptr };

		size_t bufferSize = sizeof(T) * bufferData.size();

		// Commited resource that acts as the destination resource.
		CD3DX12_HEAP_PROPERTIES defaultHeapProperites(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, resourceFlags);
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperites, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&destinationResource)));

		// Intermediate upload heap that is needed to transfer CPU buffer data into the GPU memory.
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC intermediateResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &intermediateResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource)));

		// Logic to transfer data from CPU to GPU.
		D3D12_SUBRESOURCE_DATA subresourceData
		{
			.pData = bufferData.data(),
			.RowPitch = static_cast<long long>(bufferSize),
			.SlicePitch = static_cast<long long>(bufferSize)
		};

		UpdateSubresources(commandList, destinationResource, intermediateResource, 0u, 0u, 1u, &subresourceData);

		return { destinationResource, intermediateResource };
	}
}