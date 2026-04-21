#pragma once
#include "pch.h"
#include "DescriptorHeap.h"
#include "Texture.h"

namespace sludge
{
	class TextureUAV
	{
	public:
		void CreateUAVFromTexture(ID3D12Device* device, DescriptorHeap& heap, Texture& texture, std::wstring_view name, int index = 0);
		ID3D12Resource* TextureResource() const;

	private:
		void* textureData_{};
		int width_{};
		int height_{};
		int pixelChannels_{};

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle_;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle_;

	};
} // sludge