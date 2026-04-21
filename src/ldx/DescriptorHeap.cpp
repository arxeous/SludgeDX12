#include "pch.h"
#include "DescriptorHeap.h"

namespace sludge
{
	void DescriptorHeap::CreateDescriptorHeap(ID3D12Device* const device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t descriptorCount, std::wstring_view name)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc
		{
			.Type = type,
			.NumDescriptors = descriptorCount,
			.Flags = flags,
			.NodeMask = 0
		};
		flags_ = flags;
		ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap_)));
		descriptorHeap_->SetName(name.data());
		descriptorHandleEnd_.CpuDescriptorHandle = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		//cpuEnd_ = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
		if (flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			//gpuEnd_ = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
			descriptorHandleEnd_.GpuDescriptorHandle = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
		}
		
		descriptorSize_ = device->GetDescriptorHandleIncrementSize(type);

	}
	void DescriptorHeap::OffsetHandles(uint32_t idx)
	{
		descriptorHandleEnd_.CpuDescriptorHandle = descriptorHandleEnd_.CpuDescriptorHandle.Offset(idx, descriptorSize_);
		if (descriptorHandleEnd_.GpuDescriptorHandle.ptr != 0)
		{
			descriptorHandleEnd_.GpuDescriptorHandle = descriptorHandleEnd_.GpuDescriptorHandle.Offset(idx, descriptorSize_);
		}

	}
	D3D12_DESCRIPTOR_HEAP_FLAGS DescriptorHeap::HeapFlags()
	{
		return flags_;
	}
}// sludge