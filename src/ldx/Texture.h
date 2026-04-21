#pragma once
#include "pch.h"
#include "utils/utils.h"
#include "utils/Pool.h"
#include "utils/Holder.h"
#include "DescriptorHeap.h"

namespace sludge
{
	class Texture
	{
	public:
		// Listen I know that I could create constructors isntead of these functions, but explicitly calling things just makes things easier for me to read.
		// Will this bite me in the ass later down the line? Maybe. But its my engine and if you want to learn anything you gotta
		// make mistakes, so if and when I finally get to the point where this is a problem ill learn and fix it. Simple as.
		void CreateTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& heap, std::string_view file, std::wstring_view name, utils::Pool<utils::TextureTag, DescriptorHandle>& pool,
			bool isSRGB = true);
		void CreateEmptyTexture(ID3D12Device* device, DescriptorHeap& heap, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels,
			DXGI_FORMAT format, std::wstring_view name, utils::Pool<utils::TextureTag, DescriptorHandle>& pool, bool allowUAV = false);
		void CreateHDRTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& heap, std::string_view file,
			uint32_t mipLevels, DXGI_FORMAT format, std::wstring_view name, utils::Pool<utils::TextureTag, DescriptorHandle>& pool);
		void CreateMipLevelTexture(ID3D12Device* device, DescriptorHeap& heap, utils::Pool<utils::TextureTag, DescriptorHandle>& pool, uint16_t level);
		uint16_t GetMipMapLevelIndex(uint16_t level);
		int Width();
		int Height();
		DXGI_FORMAT Format();
		utils::Holder<utils::TextureHandle>& SRVHandle();
		utils::Holder<utils::TextureHandle>& UAVHandle();
		CD3DX12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool);
		CD3DX12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool);
		ID3D12Resource* TextureResource();
	private:
		unsigned char* texData_{};
		float* HDRData_{};
		int width_{};
		int height_{};
		// This is especially important for setting up the row pitch/slice pitch, etc.
		int pixelChannels_{};

		DXGI_FORMAT format_{};

		Microsoft::WRL::ComPtr<ID3D12Resource> textureBuffer_{};
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateBuffer_{};

		utils::Holder<utils::TextureHandle> srvHandle_{};
		utils::Holder<utils::TextureHandle> uavHandle_{};
		std::vector<utils::Holder<utils::TextureHandle>> mipMapLevels{};
		//CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle_;
		//CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle_;

	};
}