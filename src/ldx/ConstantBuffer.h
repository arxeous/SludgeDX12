#pragma once
#include "pch.h"
#include "DescriptorHeap.h"
#include "utils/Handle.h"
#include "utils/Holder.h"
#include "utils/Pool.h"

// Since this thing is a templated class, the definitions of the functions that are dependent on type T must all be in the same place
// i.e. the header file!
namespace sludge
{
	// Works a lot like an upload buffer from Frank lunas book, just changed abit to be for constant buffers exclusively.
	template <typename T>
	class ConstantBuffer
	{
	public:
		void CreateConstantBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const cmdList, const T&& data,
			DescriptorHeap& heap, std::wstring_view name, int index = 0)
		{
			CD3DX12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC cbufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T));

			ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProp, D3D12_HEAP_FLAG_NONE, &cbufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cBuffer_)));
			cBuffer_->SetName(name.data());
			// You can do one of two things. Here we actually create a cbv, with the address and size of the the data. IN uploadbuffer we dont create a cbv at all,
			// and we just return the GPU address of the item.
			cBufferDesc_ =
			{
				.BufferLocation = cBuffer_->GetGPUVirtualAddress(),
				.SizeInBytes = sizeof(T)
			};

			auto heapLocation = heap.CPUDescriptorHandleStart().Offset(index, heap.IncrementSize());
			device->CreateConstantBufferView(&cBufferDesc_, heapLocation);

			CD3DX12_RANGE readRange(0, 0);
			ThrowIfFailed(cBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&dataPtr_)));
			void* dataPtr = (void*)(&data);
			memcpy(dataPtr_, dataPtr, sizeof(T));

			bufferData_ = data;
		}
		D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBufferViewDesc()
		{
			return cBufferDesc_;
		}

		void UpdateBuffer()
		{
			memcpy(dataPtr_, &bufferData_, sizeof(T));
		}

		T& ConstantBufferData()
		{
			return bufferData_;
		}

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> cBuffer_;
		D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferDesc_;
		// Raw data
		T bufferData_;
		// A pointer to a memory block that receives a pointer to the resource data. basically a pointer to the block where we upload our raw data
		void* dataPtr_{ nullptr };
	};
} // sludge