#include "pch.h"
#include "Texture.h"

// Gotta love stb libraries!!
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"


namespace sludge
{
	void Texture::CreateTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& heap, std::string_view file, std::wstring_view name, utils::Pool<utils::TextureTag, DescriptorHandle>& pool,
		bool isSRGB)
	{
		void* image = stbi_load(file.data(), &width_, &height_, &pixelChannels_, 4);
		format_ = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
		// by default were assuming that everythings already in srgb, just to make gamma correction simpler to deal with.
		D3D12_RESOURCE_DESC texDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = static_cast<UINT>(width_),
			.Height = static_cast<UINT>(height_),
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = format_,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0,
			},
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};

		// Prepare our actual texture to take in the data.
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties{ D3D12_HEAP_TYPE_DEFAULT };
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureBuffer_)));

		// Create the upload heap
		const uint64_t uploadBufferSize = GetRequiredIntermediateSize(textureBuffer_.Get(), 0, 1);
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties{ D3D12_HEAP_TYPE_UPLOAD };
		CD3DX12_RESOURCE_DESC uploadHeapDesc{ CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize) };
		ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadHeapDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateBuffer_)));

		D3D12_SUBRESOURCE_DATA texSubresourceData
		{
			.pData = image,
			.RowPitch = width_ * 4,
			.SlicePitch = height_ * width_ * 4
		};

		// Theres two ways to update buffer info. UpdateSubresources (for stuff that remains static) and Map(), UnMap() for dynamic stuff that changes everyframe. 
		UpdateSubresources(cmdList, textureBuffer_.Get(), intermediateBuffer_.Get(), 0, 0, 1, &texSubresourceData);
		stbi_image_free(image);
		textureBuffer_->SetName(name.data());

		//Transition to be now read as a srv
		utils::TransitionResource(cmdList, textureBuffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = texDesc.Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D
			{
				.MostDetailedMip = 0,
				.MipLevels = 1
			}
		};

		srvHandle_ = pool.create(DescriptorHandle{});
		auto heapLocationCPU = heap.CPUDescriptorHandleStart().Offset(srvHandle_.index(), heap.IncrementSize());
		auto descriptorHandle = pool.get(srvHandle_);
		device->CreateShaderResourceView(textureBuffer_.Get(), &srvDesc, heapLocationCPU);
		descriptorHandle->CpuDescriptorHandle = heapLocationCPU;

		if (heap.HeapFlags() == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			descriptorHandle->GpuDescriptorHandle = heap.GPUDescriptorHandleStart().Offset(srvHandle_.index(), heap.IncrementSize());
		}
		
	}

	void Texture::CreateEmptyTexture(ID3D12Device* device, DescriptorHeap& heap, 
		uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, DXGI_FORMAT format, std::wstring_view name, utils::Pool<utils::TextureTag, DescriptorHandle>& pool, bool allowUAV)
	{
		width_ = width;
		height_ = height;
		format_ = format;
		D3D12_RESOURCE_DESC texDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = static_cast<UINT>(width_),
			.Height = static_cast<UINT>(height_),
			.DepthOrArraySize = static_cast<UINT16>(depth),
			.MipLevels = static_cast<UINT16>(mipLevels),
			.Format = format_,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0,
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = allowUAV ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE,
		};

		CD3DX12_HEAP_PROPERTIES defaultHeapProperties{ D3D12_HEAP_TYPE_DEFAULT };
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&textureBuffer_)));
		textureBuffer_->SetName(name.data());


		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		if (depth == 1)
		{
			srvDesc =
			{
				.Format = format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D
				{
					.MostDetailedMip = 0,
					.MipLevels = 1
				}
			};
		}
		else if (depth == 6)
		{
			srvDesc =
			{
				.Format = format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D
				{
					.MostDetailedMip = 0,
					.MipLevels = 1
				}
			};
			// Currently we only generate mip maps for the prefiltered env map, so we set its mip level to 6 here. 
			// at some point well implement compute mip map generation and just default set them to 6 above.
			if (mipLevels == 6)
			{
				srvDesc.Texture2D.MipLevels = mipLevels;
			}
		}


		srvHandle_ = pool.create(DescriptorHandle{});
		auto descriptorHandle = pool.get(srvHandle_);
		auto heapLocationCPU = heap.CPUDescriptorHandleStart().Offset(srvHandle_.index(), heap.IncrementSize());
		device->CreateShaderResourceView(textureBuffer_.Get(), &srvDesc, heapLocationCPU);

		descriptorHandle->CpuDescriptorHandle = heapLocationCPU;
		if (heap.HeapFlags() == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			descriptorHandle->GpuDescriptorHandle = heap.GPUDescriptorHandleStart().Offset(srvHandle_.index(), heap.IncrementSize());
		}

		// If the texture has a uav counterpart, create it here
		if (allowUAV)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};

			if (depth == 1)
			{
				uavDesc =
				{
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
					.Texture2D
					{
						.MipSlice = 0
					}
				};
			}
			else if (depth == 6)
			{
				uavDesc =
				{
					.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
				.Texture2DArray
				{
					.MipSlice = 0,
					.FirstArraySlice = 0,
					.ArraySize = 6
				}
				};
			}

			uavHandle_ = pool.create(DescriptorHandle{});
			descriptorHandle = pool.get(uavHandle_);
			heapLocationCPU = heap.CPUDescriptorHandleStart().Offset(uavHandle_.index(), heap.IncrementSize());
			device->CreateUnorderedAccessView(textureBuffer_.Get(), nullptr, &uavDesc, heapLocationCPU);
			descriptorHandle->CpuDescriptorHandle = heapLocationCPU;
			if (heap.HeapFlags() == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
			{
				descriptorHandle->GpuDescriptorHandle = heap.GPUDescriptorHandleStart().Offset(uavHandle_.index(), heap.IncrementSize());
			}
		}

	}
	void Texture::CreateHDRTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, DescriptorHeap& heap,
		std::string_view file, uint32_t mipLevels, DXGI_FORMAT format, std::wstring_view name, utils::Pool<utils::TextureTag, DescriptorHandle>& pool)
	{
		HDRData_ = stbi_loadf(file.data(), &width_, &height_, &pixelChannels_, 4);
		pixelChannels_ = 4;
		format_ = format;
		D3D12_RESOURCE_DESC texDesc
		{
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Width = static_cast<UINT>(width_),
			.Height = static_cast<UINT>(height_),
			.DepthOrArraySize = 1,
			.MipLevels = static_cast<UINT16>(mipLevels),
			.Format = format_,
			.SampleDesc
			{
				.Count = 1,
				.Quality = 0,
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};

		// Prepare our actual texture to take in the data.
		CD3DX12_HEAP_PROPERTIES defaultHeapProperties{ D3D12_HEAP_TYPE_DEFAULT };
		ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&textureBuffer_)));

		// Create the upload heap
		const uint64_t uploadBufferSize = GetRequiredIntermediateSize(textureBuffer_.Get(), 0, 1);
		CD3DX12_HEAP_PROPERTIES uploadHeapProperties{ D3D12_HEAP_TYPE_UPLOAD };
		CD3DX12_RESOURCE_DESC uploadHeapDesc{ CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize) };
		ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadHeapDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateBuffer_)));

		D3D12_SUBRESOURCE_DATA texSubresourceData
		{
			.pData = HDRData_,
			.RowPitch = (width_ * pixelChannels_ * 4) ,
			.SlicePitch = (height_ * width_ * pixelChannels_ * 4)
		};

		UpdateSubresources(cmdList, textureBuffer_.Get(), intermediateBuffer_.Get(), 0, 0, 1, &texSubresourceData);
		//stbi_image_free(image);
		textureBuffer_->SetName(name.data());

		//Transition to be now read as a srv
		utils::TransitionResource(cmdList, textureBuffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			.Format = texDesc.Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D
			{
				.MipLevels = 1
			}
		};

		srvHandle_ = pool.create(DescriptorHandle{});
		auto descriptorHandle = pool.get(srvHandle_);
		auto heapLocationCPU = heap.CPUDescriptorHandleStart().Offset(srvHandle_.index(), heap.IncrementSize());
		device->CreateShaderResourceView(textureBuffer_.Get(), &srvDesc, heapLocationCPU);
		descriptorHandle->CpuDescriptorHandle = heapLocationCPU;

		if (heap.HeapFlags() == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			descriptorHandle->GpuDescriptorHandle = heap.GPUDescriptorHandleStart().Offset(srvHandle_.index(), heap.IncrementSize());
		}
	}

	void Texture::CreateMipLevelTexture(ID3D12Device* device, DescriptorHeap& heap, utils::Pool<utils::TextureTag, DescriptorHandle>& pool, uint16_t level)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		auto desc = textureBuffer_->GetDesc();

		if (desc.DepthOrArraySize == 1)
		{
			uavDesc =
			{
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D
				{
					.MipSlice = level
				}
			};
		}
		else if (desc.DepthOrArraySize == 6)
		{

			uavDesc =
			{
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
				.Texture2DArray
				{
					.MipSlice = level,
					.FirstArraySlice = 0,
					.ArraySize = 6
				}
			};
		}

		auto mipLevelUav = pool.create(DescriptorHandle{});
		auto descriptorHandle = pool.get(mipLevelUav);

		auto heapLocationCPU = heap.CPUDescriptorHandleStart().Offset(mipLevelUav.index(), heap.IncrementSize());
		device->CreateUnorderedAccessView(textureBuffer_.Get(), nullptr, &uavDesc, heapLocationCPU);
		descriptorHandle->CpuDescriptorHandle = heapLocationCPU;
		if (heap.HeapFlags() == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			descriptorHandle->GpuDescriptorHandle = heap.GPUDescriptorHandleStart().Offset(uavHandle_.index(), heap.IncrementSize());
		}
		mipMapLevels.push_back(mipLevelUav);

	}
	uint16_t Texture::GetMipMapLevelIndex(uint16_t level)
	{
		if (level == 0)
		{
			return uavHandle_.index();
		}
		else
		{
			return mipMapLevels.at(level - 1).index();
		}
	}
	int Texture::Width()
	{
		return width_;
	}
	int Texture::Height()
	{
		return height_;
	}
	DXGI_FORMAT Texture::Format()
	{
		return format_;
	}
	utils::Holder<utils::TextureHandle>& Texture::SRVHandle()
	{
		return srvHandle_;
	}
	utils::Holder<utils::TextureHandle>& Texture::UAVHandle()
	{
		return uavHandle_;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE Texture::CPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool)
	{
		auto descriptorHandle = pool.get(srvHandle_);
		return descriptorHandle->CpuDescriptorHandle;
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE Texture::GPUDescriptorHandle(utils::Pool<utils::TextureTag, DescriptorHandle>& pool)
	{
		auto descriptorHandle = pool.get(srvHandle_);
		return descriptorHandle->GpuDescriptorHandle;
	}
	ID3D12Resource* Texture::TextureResource()
	{
		return textureBuffer_.Get();
	}
}