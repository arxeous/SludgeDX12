#pragma once
#include "pch.h"
#include "DescriptorHeap.h"
namespace sludge
{
	class DepthStencilBuffer
	{
	public:
		void CreateDepthStencilBuffer(ID3D12Device* const device, DescriptorHeap dsvHeap, uint32_t width, uint32_t height, std::wstring_view name);
		CD3DX12_CPU_DESCRIPTOR_HANDLE BufferHandle() { return bufferHandle_; };
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_{};
		CD3DX12_CPU_DESCRIPTOR_HANDLE bufferHandle_{};
	};
} // sludge