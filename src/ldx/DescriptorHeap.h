#pragma once
#include "pch.h"

namespace sludge
{
	struct DescriptorHandle
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle{};
		CD3DX12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle{};
	};
	// Wrapper for the descriptor heap and its relevant data.
	class DescriptorHeap
	{
	public:
		DescriptorHeap()
		{
		};

		void CreateDescriptorHeap(ID3D12Device* const device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t descriptorCount, std::wstring_view name);

		
		[[nodiscard]] ID3D12DescriptorHeap* GetDescriptorHeap() { return descriptorHeap_.Get(); };
		uint32_t IncrementSize() { return descriptorSize_; };

		// I honestly dont know why these werent already CD3DX12_XPU_DESCRIPTOR_HANDLE before, they literally have all the functionality for offsets and etc. built into them.
		[[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandleStart() { return CD3DX12_CPU_DESCRIPTOR_HANDLE{ descriptorHeap_->GetCPUDescriptorHandleForHeapStart() }; };
		[[nodiscard]] CD3DX12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandleStart() { return CD3DX12_GPU_DESCRIPTOR_HANDLE{ descriptorHeap_->GetGPUDescriptorHandleForHeapStart() }; };

		// Deprecated for the MOST part. The only thing that uses this is the swap chain I think? Everything else keeps track of its handles and location in the descriptor heap through 
		// memory pools. 
		[[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandleEnd(){ return descriptorHandleEnd_.CpuDescriptorHandle; };
		[[nodiscard]] CD3DX12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandleEnd() { return descriptorHandleEnd_.GpuDescriptorHandle; };
		void OffsetHandles(uint32_t idx = 1);
		D3D12_DESCRIPTOR_HEAP_FLAGS HeapFlags();

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
		//CD3DX12_CPU_DESCRIPTOR_HANDLE cpuEnd_{};
		//CD3DX12_GPU_DESCRIPTOR_HANDLE gpuEnd_{};
		DescriptorHandle descriptorHandleEnd_{};

		D3D12_DESCRIPTOR_HEAP_FLAGS flags_{};
		uint32_t descriptorSize_{};


	};
} // sludge
